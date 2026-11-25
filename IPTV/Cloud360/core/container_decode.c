#include "container_decode.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mem.h>
#include <libavutil/error.h>
#include <libavutil/dict.h>
enum { MAX_BYTES=8*1024*1024, MAX_PACKETS=4096, MAX_FRAMES=1200, EXTENDED_BYTES=256*1024*1024, EXTENDED_PACKETS=250000, EXTENDED_FRAMES=180000 };
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
typedef struct { CloudContainerFrame cb; void* user; AVRational timebase; unsigned max_frames; } Sink;
static int drain(AVCodecContext* ctx,AVFrame* frame,CloudContainerReport* r,int video,Sink sink){
    for(;;){
        int rc=avcodec_receive_frame(ctx,frame);
        if(rc==AVERROR(EAGAIN)||rc==AVERROR_EOF)return 0;
        if(rc<0)return rc;
        if(video){
            if(frame->width<=0||frame->height<=0||frame->width>1280||frame->height>720||++r->video_frames>sink.max_frames)return AVERROR_INVALIDDATA;
            r->width=(unsigned)frame->width;r->height=(unsigned)frame->height;
        }else{
            if(frame->nb_samples<=0||frame->nb_samples>8192||++r->audio_frames>sink.max_frames)return AVERROR_INVALIDDATA;
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
struct CloudContainerSession {
    Input input;
    AVFormatContext* fmt;
    AVIOContext* io;
    AVCodecContext *video,*audio;
    AVPacket* pkt;
    AVFrame* frame;
    Sink vs,as;
    CloudContainerReport report;
    unsigned count,max_packets;
    int vi,ai,state;
};
void cloud_container_close(CloudContainerSession* s){
    if(!s)return;
    av_packet_free(&s->pkt);av_frame_free(&s->frame);
    avcodec_free_context(&s->video);avcodec_free_context(&s->audio);
    avformat_close_input(&s->fmt);
    if(s->io){av_freep(&s->io->buffer);avio_context_free(&s->io);}
    if(s->input.file)fclose(s->input.file);
    free(s);
}
CloudContainerSession* cloud_container_open(const char* path,CloudContainerFrame output,void* user,int extended){
    if(!path||strstr(path,"://"))return NULL;
    CloudContainerSession* s=calloc(1,sizeof *s);if(!s)return NULL;
    const int64_t max_bytes=extended?EXTENDED_BYTES:MAX_BYTES;
    s->input.budget=max_bytes*4;
    s->max_packets=extended?EXTENDED_PACKETS:MAX_PACKETS;
    s->input.file=fopen(path,"rb");
    AVDictionary* options=NULL;
    uint8_t* buffer=NULL;
    if(!s->input.file||fseek(s->input.file,0,SEEK_END)||
       ((s->input.length=ftell(s->input.file))<=0)||s->input.length>max_bytes||
       fseek(s->input.file,0,SEEK_SET))goto fail;
    buffer=av_malloc(32768);if(!buffer)goto fail;
    s->io=avio_alloc_context(buffer,32768,0,&s->input,read_input,NULL,seek_input);
    if(!s->io)goto fail;buffer=NULL;
    s->fmt=avformat_alloc_context();if(!s->fmt)goto fail;
    s->fmt->pb=s->io;s->fmt->flags|=AVFMT_FLAG_CUSTOM_IO;s->fmt->io_open=deny_open;
    s->fmt->probesize=256*1024;s->fmt->max_analyze_duration=2000000;s->fmt->max_streams=8;
    av_dict_set(&options,"format_whitelist","mov,mp4,m4a,3gp,3g2,mj2,mpegts",0);
    av_dict_set(&options,"protocol_whitelist","file",0);
    if(avformat_open_input(&s->fmt,NULL,NULL,&options)<0||avformat_find_stream_info(s->fmt,NULL)<0)goto fail;
    s->vi=av_find_best_stream(s->fmt,AVMEDIA_TYPE_VIDEO,-1,-1,NULL,0);
    s->ai=av_find_best_stream(s->fmt,AVMEDIA_TYPE_AUDIO,-1,-1,NULL,0);
    if(s->vi<0||s->ai<0)goto fail;
    s->video=open_codec(s->fmt->streams[s->vi],AV_CODEC_ID_H264);
    s->audio=open_codec(s->fmt->streams[s->ai],AV_CODEC_ID_AAC);
    if(!s->video||!s->audio)goto fail;
    s->pkt=av_packet_alloc();s->frame=av_frame_alloc();if(!s->pkt||!s->frame)goto fail;
    s->vs=(Sink){output,user,s->fmt->streams[s->vi]->time_base,extended?EXTENDED_FRAMES:MAX_FRAMES};
    s->as=(Sink){output,user,s->fmt->streams[s->ai]->time_base,extended?EXTENDED_FRAMES:MAX_FRAMES};
    s->state=1;av_dict_free(&options);return s;
fail:
    av_free(buffer);av_dict_free(&options);cloud_container_close(s);return NULL;
}
const CloudContainerReport* cloud_container_report(const CloudContainerSession* s){return s?&s->report:NULL;}
int cloud_container_step(CloudContainerSession* s,unsigned packet_budget){
    if(!s||!packet_budget||packet_budget>64)return AVERROR(EINVAL);
    if(s->state<=0)return s->state;
    for(unsigned i=0;i<packet_budget;++i){
        int read=av_read_frame(s->fmt,s->pkt);
        if(read==AVERROR_EOF){
            int rc=submit(s->video,NULL,s->frame,&s->report,1,s->vs);
            if(rc>=0)rc=submit(s->audio,NULL,s->frame,&s->report,0,s->as);
            s->state=rc<0?rc:s->report.video_frames&&s->report.audio_samples?0:AVERROR_INVALIDDATA;
            return s->state;
        }
        if(read<0||++s->count>s->max_packets){s->state=read<0?read:AVERROR_INVALIDDATA;return s->state;}
        int rc=s->pkt->stream_index==s->vi?submit(s->video,s->pkt,s->frame,&s->report,1,s->vs):
            s->pkt->stream_index==s->ai?submit(s->audio,s->pkt,s->frame,&s->report,0,s->as):0;
        av_packet_unref(s->pkt);
        if(rc<0){s->state=rc;return rc;}
    }
    return 1;
}
int cloud_container_decode_output(const char* path,CloudContainerReport* report,CloudContainerFrame output,void* user){
    if(!report)return -1;
    memset(report,0,sizeof *report);
    CloudContainerSession* s=cloud_container_open(path,output,user,0);
    if(!s)return -2;
    int rc;
    do{rc=cloud_container_step(s,16);}while(rc>0);
    *report=s->report;cloud_container_close(s);return rc;
}
int cloud_container_decode(const char* path,CloudContainerReport* report){return cloud_container_decode_output(path,report,NULL,NULL);}
