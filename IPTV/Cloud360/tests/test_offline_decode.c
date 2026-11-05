#include "offline_decode.h"
#include <stdio.h>
#include <string.h>
int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <file> <h264|aac|mp3|mpeg2|ac3>\n", argv[0]);
        return 2;
    }
    CloudCodec codec = CLOUD_CODEC_H264;
    if (!strcmp(argv[2], "aac")) codec = CLOUD_CODEC_AAC;
    else if (!strcmp(argv[2], "mp3")) codec = CLOUD_CODEC_MP3;
    else if (!strcmp(argv[2], "mpeg2")) codec = CLOUD_CODEC_MPEG2_VIDEO;
    else if (!strcmp(argv[2], "ac3")) codec = CLOUD_CODEC_AC3;
    else if (strcmp(argv[2], "h264")) return 2;
    CloudDecodeReport result;
    int rc = cloud_offline_decode_file(argv[1], codec, &result);
    printf("codec=%s rc=%d frames=%u samples=%u dimensions=%ux%u bytes=%llu\n",
           cloud_codec_name(codec), rc, result.frames,
           result.audio_sample_frames, result.video_width, result.video_height,
           (unsigned long long)result.bytes_read);
    if (rc || result.frames < 1)
        return 1;
    if ((codec == CLOUD_CODEC_H264 || codec == CLOUD_CODEC_MPEG2_VIDEO) &&
        (result.video_width == 0 || result.video_height == 0))
        return 1;
    if ((codec == CLOUD_CODEC_AAC || codec == CLOUD_CODEC_MP3 ||
         codec == CLOUD_CODEC_AC3) && result.audio_sample_frames == 0)
        return 1;
    return 0;
}
