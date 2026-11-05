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

/* Skip ID3v2 metadata that a container demuxer would normally strip.
 * The elementary MP3 parser expects MPEG audio frames, not an ID3 tag.
 */
static int skip_id3v2(FILE *file)
{
    unsigned char h[10];
    size_t n = fread(h, 1, sizeof h, file);
    if (n < sizeof h || memcmp(h, "ID3", 3) != 0) {
        return fseek(file, 0, SEEK_SET) == 0 ? 0 : -1;
    }
    /* Each tag size byte is a 7-bit "synchsafe" integer. */
    for (unsigned i = 6; i < 10; i++)
        if (h[i] & 0x80) return -1;
    unsigned long tag_size =
        ((unsigned long)h[6] << 21) | ((unsigned long)h[7] << 14) |
        ((unsigned long)h[8] << 7) | (unsigned long)h[9];
    unsigned long offset = 10u + tag_size + ((h[5] & 0x10) ? 10u : 0u);
    if (offset > 1024u * 1024u) return -1;
    return fseek(file, (long)offset, SEEK_SET) == 0 ? 0 : -1;
}
int cloud_offline_decode_file(const char *path, CloudCodec codec, CloudDecodeReport *report)
{
    if (!path || !report)
        return -1;
    memset(report, 0, sizeof *report);
    FILE *file = fopen(path, "rb");
    if (!file)
        return -2;
    if (codec == CLOUD_CODEC_MP3 && skip_id3v2(file) != 0) {
        fclose(file);
        return -9;
    }
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
