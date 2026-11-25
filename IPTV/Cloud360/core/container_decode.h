#ifndef CLOUD360_CONTAINER_DECODE_H
#define CLOUD360_CONTAINER_DECODE_H
#include <stdint.h>
typedef struct {
    unsigned video_frames, audio_frames, width, height;
    uint64_t audio_samples;
} CloudContainerReport;
/* Bounded local diagnostic, H.264/AAC in MP4 or MPEG-TS.
 * No URL, playlists, output, DRM, or unrestricted demuxing. */
int cloud_container_decode(const char* path, CloudContainerReport* report);
#endif
