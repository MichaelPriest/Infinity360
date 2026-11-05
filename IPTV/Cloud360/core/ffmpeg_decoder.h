#ifndef CLOUD360_FFMPEG_DECODER_H
#define CLOUD360_FFMPEG_DECODER_H
#include <stddef.h>
#include <stdint.h>
#include <libavutil/frame.h>
#include "codec_probe.h"
typedef struct CloudVideoDecoder CloudVideoDecoder;
typedef void (*CloudDecodedFrame)(void *user, CloudCodec codec, const AVFrame *frame);
/* Software decode of elementary stream only. Caller must demux PES/HLS/MP4.
   Callback must copy/ref AVFrame before returning. Not DRM/hardware support. */
CloudVideoDecoder *cloud_decoder_open(CloudCodec codec, CloudDecodedFrame cb, void *user);
int cloud_decoder_feed(CloudVideoDecoder *decoder, const uint8_t *data, size_t size);
int cloud_decoder_finish(CloudVideoDecoder *decoder);
void cloud_decoder_close(CloudVideoDecoder *decoder);
#endif
