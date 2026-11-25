#include "container_decode.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mem.h>
#include <libavutil/error.h>
#include <libavutil/dict.h>
enum { MAX_BYTES=8*1024*1024, MAX_PACKETS=4096, MAX_FRAMES=1200 };
typedef struct { FILE* file; int64_t length; int64_t budget; } Input;
static int read_input(void* opaque,uint8_t* buf,int n){
    Input* in=opaque;
    if(n<=0||in->budget<=0)return AVERROR(EIO);
    if(n>in->budget)n=(int)in->budget;
    size_t got=fread(buf,1,(size_t)n,in->file);
    in->budget-=got;
    return got?(int)got:ferror(in->file)?AVERROR(EIO):AVERROR_EOF;
}
static int64_t seek_input(void* opaque,int64_t offset,int whence){
    Input* in=opaque;
    if(whence==AVSEEK_SIZE)return in->length;
    whence&=~AVSEEK_FORCE;
    int64_t pos=whence==SEEK_SET?0:whence==SEEK_CUR?ftell(in->file):whence==SEEK_END?in->length:-1;
    if(pos<0||offset < -pos||offset > in->length-pos)return AVERROR(EINVAL);
    pos+=offset;
    return fseek(in->file,(long)pos,SEEK_SET)==0?pos:AVERROR(EIO);
}
static int deny_open(AVFormatContext* s, AVIOContext** pb,const char* url,int flags,AVDictionary** opts){
    (void)s;(void)pb;(void)url;(void)flags;(void)opts;return AVERROR(EACCES);
}
static AVCodecContext* open_codec(AVStream* stream,enum AVCodecID expected){
    if(stream->codecpar->codec_id!=expected)return NULL;
    if(expected==AV_CODEC_ID_H264&&(stream->codecpar->width>1280||stream->codecpar->height>720))return NULL;
    const AVCodec* codec=avcodec_find_decoder(expected);
    if(!codec)return NULL;
    AVCodecContext* ctx=avcodec_alloc_context3(codec);
    if(!ctx)return NULL;
    if(avcodec_parameters_to_context(ctx,stream->codecpar)<0){avcodec_free_context(&ctx);return NULL;}
    ctx->thread_count=1;ctx->max_pixels=1280*720;
    if(avcodec_open2(ctx,codec,NULL)<0){avcodec_free_context(&ctx);return NULL;}
    return ctx;
}
typedef struct { CloudContainerFrame cb; void* user; AVRational timebase; } Sink;
static int drain(AVCodecContext* ctx,AVFrame* frame,CloudContainerReport* r,int video,Sink sink){
    for(;;){
        int rc=avcodec_receive_frame(ctx,frame);
        if(rc==AVERROR(EAGAIN)||rc==AVERROR_EOF)return 0;
        if(rc<0)return rc;
        if(video){
            if(frame->width<=0||frame->height<=0||frame->width>1280||frame->height>720||++r->video_frames>MAX_FRAMES)return AVERROR_INVALIDDATA;
            r->width=(unsigned)frame->width;r->height=(unsigned)frame->height;
        }else{
            if(frame->nb_samples<=0||frame->nb_samples>8192||++r->audio_frames>MAX_FRAMES)return AVERROR_INVALIDDATA;
            r->audio_samples+=(unsigned)frame->nb_samples;
        }
        if(sink.cb){
            int64_t pts=frame->best_effort_timestamp;
            if(pts!=AV_NOPTS_VALUE)pts=av_rescale_q(pts,sink.timebase,(AVRational){1,1000000});
            if(sink.cb(sink.user,frame,video,pts)!=0){av_frame_unref(frame);return AVERROR_EXIT;}
        }
        av_frame_unref(frame);
    }
}
static int submit(AVCodecContext* ctx,AVPacket* pkt,AVFrame* frame,CloudContainerReport* r,int video,Sink sink){
    int rc=avcodec_send_packet(ctx,pkt);
    if(rc==AVERROR(EAGAIN)){
        rc=drain(ctx,frame,r,video,sink);
        if(rc<0)return rc;
        rc=avcodec_send_packet(ctx,pkt);
    }
    if(rc<0&&rc!=AVERROR_EOF)return rc;
    return drain(ctx,frame,r,video,sink);
}
int cloud_container_decode_output(const char* path,CloudContainerReport* report,CloudContainerFrame output,void* user){
    if(!path||!report||strstr(path,"://"))return -1;
    memset(report,0,sizeof *report);
    FILE* file=fopen(path,"rb");if(!file)return -2;
    Input in={file,0,MAX_BYTES*4};
    int rc=-3;
    AVFormatContext* fmt=NULL;AVIOContext* io=NULL;uint8_t* buffer=NULL;
    AVCodecContext *video=NULL,*audio=NULL;AVPacket* pkt=NULL;AVFrame* frame=NULL;
    AVDictionary* options=NULL;
    if(fseek(file,0,SEEK_END)||((in.length=ftell(file))<=0)||in.length>MAX_BYTES||fseek(file,0,SEEK_SET))goto done;
    buffer=av_malloc(32768);if(!buffer)goto done;
    io=avio_alloc_context(buffer,32768,0,&in,read_input,NULL,seek_input);
    if(!io)goto done;buffer=NULL;
    fmt=avformat_alloc_context();if(!fmt)goto done;
    fmt->pb=io;fmt->flags|=AVFMT_FLAG_CUSTOM_IO;fmt->io_open=deny_open;
    fmt->probesize=256*1024;fmt->max_analyze_duration=2000000;fmt->max_streams=8;
    av_dict_set(&options,"format_whitelist","mov,mp4,m4a,3gp,3g2,mj2,mpegts",0);
    av_dict_set(&options,"protocol_whitelist","file",0);
    if(avformat_open_input(&fmt,NULL,NULL,&options)<0||avformat_find_stream_info(fmt,NULL)<0)goto done;
    int vi=av_find_best_stream(fmt,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    int ai=av_find_best_stream(fmt,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    if(vi<0||ai<0)goto done;
    video=open_codec(fmt->streams[vi],AV_CODEC_ID_H264);audio=open_codec(fmt->streams[ai],AV_CODEC_ID_AAC);
    if(!video||!audio)goto done;
    pkt=av_packet_alloc();frame=av_frame_alloc();if(!pkt||!frame)goto done;
    Sink vs={output,user,fmt->streams[vi]->time_base},as={output,user,fmt->streams[ai]->time_base};
    unsigned count=0;
    for(;;){
        int read=av_read_frame(fmt,pkt);
        if(read==AVERROR_EOF)break;
        if(read<0||++count>MAX_PACKETS)goto done;
        int result=pkt->stream_index==vi?submit(video,pkt,frame,report,1,vs):pkt->stream_index==ai?submit(audio,pkt,frame,report,0,as):0;
        av_packet_unref(pkt);if(result<0)goto done;
    }
    if(submit(video,NULL,frame,report,1,vs)<0||submit(audio,NULL,frame,report,0,as)<0)goto done;
    if(report->video_frames&&report->audio_samples)rc=0;
done:
    av_dict_free(&options);av_packet_free(&pkt);av_frame_free(&frame);
    avcodec_free_context(&video);avcodec_free_context(&audio);
    avformat_close_input(&fmt);
    if(io){av_freep(&io->buffer);avio_context_free(&io);}av_free(buffer);
    fclose(file);return rc;
}

int cloud_container_decode(const char* path,CloudContainerReport* report){return cloud_container_decode_output(path,report,NULL,NULL);}
