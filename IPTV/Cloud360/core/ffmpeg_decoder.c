#include "ffmpeg_decoder.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mem.h>
struct CloudVideoDecoder {
  CloudCodec codec;
  AVCodecContext *ctx;
  AVCodecParserContext *parser;
  AVPacket *pkt;
  AVFrame *frame;
  CloudDecodedFrame cb;
  void *user;
};
static enum AVCodecID codec_id(CloudCodec c) {
  switch(c) {
    case CLOUD_CODEC_H264: return AV_CODEC_ID_H264;
    case CLOUD_CODEC_MPEG2_VIDEO: return AV_CODEC_ID_MPEG2VIDEO;
    case CLOUD_CODEC_VC1: return AV_CODEC_ID_VC1;
    case CLOUD_CODEC_AAC: return AV_CODEC_ID_AAC;
    case CLOUD_CODEC_MP3: return AV_CODEC_ID_MP3;
    case CLOUD_CODEC_AC3: return AV_CODEC_ID_AC3;
    case CLOUD_CODEC_EAC3: return AV_CODEC_ID_EAC3;
    default: return AV_CODEC_ID_NONE;
  }
}
void cloud_decoder_close(CloudVideoDecoder *d) {
  if(!d) return;
  av_parser_close(d->parser);
  avcodec_free_context(&d->ctx);
  av_packet_free(&d->pkt);
  av_frame_free(&d->frame);
  free(d);
}
CloudVideoDecoder *cloud_decoder_open(CloudCodec codec, CloudDecodedFrame cb, void *user) {
  enum AVCodecID id=codec_id(codec);
  if(id==AV_CODEC_ID_NONE || !cb) return NULL;
  const AVCodec *av=avcodec_find_decoder(id);
  if(!av) return NULL;
  CloudVideoDecoder *d=calloc(1,sizeof *d);
  if(!d) return NULL;
  d->codec=codec; d->cb=cb; d->user=user;
  d->ctx=avcodec_alloc_context3(av);
  d->pkt=av_packet_alloc();
  d->frame=av_frame_alloc();
  if(!d->ctx || !d->pkt || !d->frame) {cloud_decoder_close(d);return NULL;}
  d->ctx->thread_count=2;
  if(avcodec_open2(d->ctx,av,NULL)<0) {cloud_decoder_close(d);return NULL;}
  d->parser=av_parser_init(id);
  return d;
}
static int receive(CloudVideoDecoder *d) {
  for(;;) {
    int rc=avcodec_receive_frame(d->ctx,d->frame);
    if(rc==AVERROR_EOF || rc==AVERROR(EAGAIN)) return 0;
    if(rc<0) return rc;
    d->cb(d->user,d->codec,d->frame);
    av_frame_unref(d->frame);
  }
}
static int packet(CloudVideoDecoder *d,const uint8_t *src,int len) {
  if(len<=0)return 0;
  av_packet_unref(d->pkt);
  int rc=av_new_packet(d->pkt,len);
  if(rc<0)return rc;
  memcpy(d->pkt->data,src,(size_t)len);
  rc=avcodec_send_packet(d->ctx,d->pkt);
  if(rc==AVERROR(EAGAIN)){
    rc=receive(d);
    if(rc>=0) rc=avcodec_send_packet(d->ctx,d->pkt);
  }
  av_packet_unref(d->pkt);
  if(rc<0)return rc;
  return receive(d);
}
int cloud_decoder_feed(CloudVideoDecoder *d,const uint8_t *src,size_t size) {
  if(!d || (!src && size) || size>(size_t)INT_MAX-AV_INPUT_BUFFER_PADDING_SIZE)return -1;
  if(size==0)return 0;
  uint8_t *padded=av_malloc(size+AV_INPUT_BUFFER_PADDING_SIZE);
  if(!padded)return AVERROR(ENOMEM);
  memcpy(padded,src,size);
  memset(padded+size,0,AV_INPUT_BUFFER_PADDING_SIZE);
  const uint8_t *cur=padded; int remain=(int)size,rc=0;
  if(d->parser) {
    while(remain>0) {
      uint8_t *frame_data=NULL;int frame_size=0;
      int consumed=av_parser_parse2(d->parser,d->ctx,&frame_data,&frame_size,cur,remain,
                   AV_NOPTS_VALUE,AV_NOPTS_VALUE,-1);
      if(consumed<0){rc=consumed;break;}
      if(frame_size && (rc=packet(d,frame_data,frame_size))<0) break;
      if(!consumed && !frame_size){rc=-2;break;}
      cur+=consumed;remain-=consumed;
    }
  } else rc=packet(d,cur,remain);
  av_free(padded);
  return rc;
}
int cloud_decoder_finish(CloudVideoDecoder *d) {
  if(!d)return -1;
  if(d->parser) {
    uint8_t *bytes=NULL;int len=0;
    int rc=av_parser_parse2(d->parser,d->ctx,&bytes,&len,NULL,0,
              AV_NOPTS_VALUE,AV_NOPTS_VALUE,-1);
    if(rc<0)return rc;
    if(len>0 && (rc=packet(d,bytes,len))<0)return rc;
  }
  int rc=avcodec_send_packet(d->ctx,NULL);
  if(rc<0 && rc!=AVERROR_EOF)return rc;
  return receive(d);
}
