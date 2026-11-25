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
/* Incremental local session: step returns 1 for progress, 0 at clean EOF,
 * negative on failure. The callback's frame is borrowed until it returns.
 * Close on every exit path; no network or unbounded input. */
typedef struct CloudContainerSession CloudContainerSession;
CloudContainerSession* cloud_container_open(const char* path,CloudContainerFrame output,void* user,int extended);
int cloud_container_step(CloudContainerSession* session,unsigned packet_budget);
const CloudContainerReport* cloud_container_report(const CloudContainerSession* session);
void cloud_container_close(CloudContainerSession* session);
int cloud_container_decode_output(const char* path,CloudContainerReport* report,CloudContainerFrame output,void* user);
int cloud_container_decode(const char* path, CloudContainerReport* report);
#ifdef __cplusplus
}
#endif
#endif
