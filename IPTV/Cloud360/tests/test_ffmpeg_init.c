#include "ffmpeg_decoder.h"
#include <assert.h>
#include <stdio.h>
static void handle(void *u,CloudCodec codec,const AVFrame *frame) {
    (void)u;(void)codec;(void)frame;
}
int main(void) {
  assert(!cloud_decoder_open(CLOUD_CODEC_H265,handle,NULL));
  CloudVideoDecoder *d=cloud_decoder_open(CLOUD_CODEC_H264,handle,NULL);
  assert(d);
  assert(cloud_decoder_feed(d,NULL,0)==0);
  assert(cloud_decoder_feed(d,NULL,2)==-1);
  cloud_decoder_close(d);
  puts("FFmpeg H.264 decoder bridge initialization PASS");
  return 0;
}
