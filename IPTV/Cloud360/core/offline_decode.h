#ifndef CLOUD360_OFFLINE_DECODE_H
#define CLOUD360_OFFLINE_DECODE_H
#include <stddef.h>
#include <stdint.h>
#include "codec_probe.h"
typedef struct {
    unsigned frames;
    unsigned video_width;
    unsigned video_height;
    unsigned audio_sample_frames;
    uint64_t bytes_read;
} CloudDecodeReport;
/*
 * Actual libavcodec SOFTWARE decoding from local elementary stream bytes.
 * H264 = Annex B .h264; AAC = ADTS .aac; MP3 = raw .mp3; AC3 = raw .ac3.
 * No network, container demux, audio output or GPU render.
 * Files limited to 4 MiB in the smoke-test mode.
 * 0=decoded >=1 frame; negative=error.
 */
int cloud_offline_decode_file(const char *path, CloudCodec codec, CloudDecodeReport *report);
#endif
