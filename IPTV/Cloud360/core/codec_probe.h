#ifndef CLOUD360_CODEC_PROBE_H
#define CLOUD360_CODEC_PROBE_H
#include <stddef.h>
#include <stdint.h>
typedef enum {
    CLOUD_CODEC_NONE=0, CLOUD_CODEC_MPEG2_VIDEO, CLOUD_CODEC_H264,
    CLOUD_CODEC_VC1, CLOUD_CODEC_H265, CLOUD_CODEC_AAC,
    CLOUD_CODEC_MP3, CLOUD_CODEC_AC3, CLOUD_CODEC_EAC3,
    CLOUD_CODEC_UNKNOWN
} CloudCodec;
typedef struct {
    uint16_t pmt_pid, video_pid, audio_pid;
    CloudCodec video_codec, audio_codec;
    unsigned pat_found, pmt_found, packets_scanned;
} CloudTransportInfo;
/* Scans up to max packets from a raw 188-byte MPEG-TS sequence.
   Return 1 if PMT parsed, 0 for unrecognized/incomplete TS, -1 invalid args. */
int cloud_probe_mpegts(const uint8_t *data, size_t length, CloudTransportInfo *out);
const char *cloud_codec_name(CloudCodec codec);
/* These codec families are targets of a SOFTWARE decoder port (e.g. FFmpeg).
   This is NOT evidence that an Xbox 360 hardware decoder is exposed by XSDK. */
int cloud_codec_software_candidate(CloudCodec codec);
#endif
