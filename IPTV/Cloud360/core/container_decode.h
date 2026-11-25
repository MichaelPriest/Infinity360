#ifndef CLOUD360_CONTAINER_DECODE_H
#define CLOUD360_CONTAINER_DECODE_H
#include <stdint.h>
#include <libavutil/frame.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    unsigned video_frames, audio_frames, width, height;
    uint64_t audio_samples;
} CloudContainerReport;
/* Bounded local diagnostic, H.264/AAC in MP4 or MPEG-TS.
 * No URL, playlists, output, DRM, or unrestricted demuxing. */
typedef int (*CloudContainerFrame)(void* user,const AVFrame* frame,int video,int64_t pts_us);
int cloud_container_decode_output(const char* path,CloudContainerReport* report,CloudContainerFrame output,void* user);
int cloud_container_decode(const char* path, CloudContainerReport* report);
#ifdef __cplusplus
}
#endif
#endif
