#ifndef CLOUD360_FRAME_OUTPUT_H
#define CLOUD360_FRAME_OUTPUT_H
#include <stddef.h>
#include <libavutil/frame.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded software bridge: YUV420P -> RGB24, FLTP mono/stereo -> interleaved float. */
int cloud_frame_rgb(const AVFrame* frame,unsigned char* dst,size_t bytes);
int cloud_frame_stereo(const AVFrame* frame,float* dst,size_t samples);
#ifdef __cplusplus
}
#endif
#endif
