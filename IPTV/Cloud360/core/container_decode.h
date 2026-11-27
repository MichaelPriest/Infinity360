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
/* Bounded local diagnostic, H.264/MPEG-2 video and AAC/MP2/MP3/AC-3/E-AC-3 audio in MP4 or MPEG-TS.
 * Source dimensions <=1920x1080; actual output scaling is the caller's responsibility.
 * No URL, playlists, output, DRM, or unrestricted demuxing. */
typedef int (*CloudContainerFrame)(void* user,const AVFrame* frame,int video,int64_t pts_us);
/* Incremental local session: step returns 1 for progress, 0 at clean EOF,
 * negative on failure. The callback's frame is borrowed until it returns.
 * Close on every exit path; no network or unbounded input. */
typedef struct CloudContainerSession CloudContainerSession;
/* Non-seekable MPEG-TS: caller owns reader/user until close. Return bytes,
 * 0 for clean EOF, negative for permanent error/cancel (not temporary EAGAIN). */
typedef int (*CloudContainerRead)(void* user,uint8_t* buffer,unsigned capacity);
/* Sequential MP4 (fast-start or compatible fragmented MP4), no random access. */
CloudContainerSession* cloud_container_open_mp4(CloudContainerRead read,void* reader,CloudContainerFrame output,void* user);
/* Seek callback takes an absolute byte position, returns that exact position or negative. Length must be known and stable. Caller owns reader until close. */
typedef int64_t (*CloudContainerSeek)(void* user,int64_t position);
CloudContainerSession* cloud_container_open_mp4_seekable(CloudContainerRead read,CloudContainerSeek seek,int64_t length,void* reader,CloudContainerFrame output,void* user);
CloudContainerSession* cloud_container_open_ts(CloudContainerRead read,void* reader,CloudContainerFrame output,void* user);
CloudContainerSession* cloud_container_open(const char* path,CloudContainerFrame output,void* user,int extended);
/* Indexed local or seekable callback MP4 only. Time is relative to the container start in us.
 * A successful seek lands at a preceding keyframe; caller must discard
 * previously queued frames. A failed demux seek is terminal. */
int64_t cloud_container_duration(const CloudContainerSession* session);
int64_t cloud_container_start(const CloudContainerSession* session);
int cloud_container_seek(CloudContainerSession* session,int64_t position_us);
int cloud_container_step(CloudContainerSession* session,unsigned packet_budget);
const CloudContainerReport* cloud_container_report(const CloudContainerSession* session);
void cloud_container_close(CloudContainerSession* session);
int cloud_container_decode_output(const char* path,CloudContainerReport* report,CloudContainerFrame output,void* user);
int cloud_container_decode(const char* path, CloudContainerReport* report);
#ifdef __cplusplus
}
#endif
#endif
