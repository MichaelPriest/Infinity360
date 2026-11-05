#include "offline_decode.h"
#include "ffmpeg_decoder.h"
#include <libavutil/frame.h>
#include <stdio.h>
#include <string.h>

enum { READ_CHUNK = 8192, MAX_INPUT_BYTES = 4 * 1024 * 1024 };
typedef struct {
    CloudDecodeReport *report;
    CloudCodec codec;
} DecodeObserver;

static void on_decoded_frame(void *opaque, CloudCodec codec, const AVFrame *frame)
{
    DecodeObserver *observer = (DecodeObserver *)opaque;
    CloudDecodeReport *report = observer->report;
    if (codec != observer->codec || !frame)
        return;
    report->frames++;
    if (frame->width > 0 && frame->height > 0) {
        report->video_width = (unsigned)frame->width;
        report->video_height = (unsigned)frame->height;
    }
    if (frame->nb_samples > 0)
        report->audio_sample_frames += (unsigned)frame->nb_samples;
}

int cloud_offline_decode_file(const char *path, CloudCodec codec, CloudDecodeReport *report)
{
    if (!path || !report)
        return -1;
    memset(report, 0, sizeof *report);
    FILE *file = fopen(path, "rb");
    if (!file)
        return -2;
    DecodeObserver observer = { report, codec };
    CloudVideoDecoder *decoder = cloud_decoder_open(codec, on_decoded_frame, &observer);
    if (!decoder) {
        fclose(file);
        return -3;
    }
    unsigned char chunk[READ_CHUNK];
    int result = 0;
    for (;;) {
        size_t count = fread(chunk, 1, sizeof chunk, file);
        if (count) {
            if (report->bytes_read + count > MAX_INPUT_BYTES) {
                result = -4;
                break;
            }
            report->bytes_read += count;
            if (cloud_decoder_feed(decoder, chunk, count) < 0) {
                result = -5;
                break;
            }
        }
        if (count < sizeof chunk) {
            if (ferror(file))
                result = -6;
            break;
        }
    }
    if (!result && cloud_decoder_finish(decoder) < 0)
        result = -7;
    cloud_decoder_close(decoder);
    fclose(file);
    if (!result && !report->frames)
        result = -8;
    return result;
}
