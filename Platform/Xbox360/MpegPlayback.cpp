#define PL_MPEG_IMPLEMENTATION
#include "MpegPlayback.h"
#include <vector>

namespace {
const unsigned ReadSize = 32 * 1024;
const unsigned ProbeLimit = 256 * 1024;
const unsigned StepLimit = 1024 * 1024;

bool sequenceHeader(const unsigned char* data, size_t size, size_t& offset, unsigned& width, unsigned& height) {
    while(offset + 8 <= size) {
        const unsigned char* p = data + offset++;
        if(p[0] || p[1] || p[2] != 1 || p[3] != 0xb3) continue;
        width = (p[4] << 4) | (p[5] >> 4);
        height = ((p[5] & 15) << 8) | p[6];
        return true;
    }
    return false;
}
}

void mpegStereoPcm16(const plm_samples_t& samples, short* destination) {
    for(unsigned i=0; i<samples.count*2; ++i) {
        const float value=samples.interleaved[i];
        destination[i]=value>=1 ? 32767 : value<=-1 ? -32768 : static_cast<short>(value*32767);
    }
}

MpegPlayback::MpegPlayback(MediaReader& reader, MpegOutput& output)
    : reader_(reader), output_(output), decoder_(0), budget_(StepLimit), eof_(false), sawFrame_(false) {}
MpegPlayback::~MpegPlayback() { if(decoder_) plm_destroy(decoder_); }

int MpegPlayback::Fill(unsigned char* bytes, unsigned size) {
    unsigned used = 0;
    while(used < size && !eof_ && error_.empty()) {
        int count = reader_.Read(bytes + used, size - used);
        if(count < 0 || static_cast<unsigned>(count) > size - used) error_ = "Leitura da transmissão interrompida.";
        else if(count == 0) eof_ = true;
        else used += count;
    }
    return static_cast<int>(used);
}

bool MpegPlayback::Open() {
    std::vector<unsigned char> prefix;
    size_t offset = 0;
    unsigned width = 0, height = 0;
    unsigned char block[4096];
    // Reject HTML, playlists and unsupported containers before starting a codec.
    // Read the sequence dimensions before allocating decoded surfaces.
    while(!eof_ && error_.empty() && prefix.size() < ProbeLimit) {
        int n = Fill(block, prefix.empty() ? 8 : sizeof block);
        prefix.insert(prefix.end(), block, block+n);
        if(prefix.size() < 5 || prefix[0] || prefix[1] || prefix[2] != 1 || prefix[3] != 0xba ||
           (prefix[4] & 0xf0) != 0x20) break;
        if(sequenceHeader(&prefix[0], prefix.size(), offset, width, height)) break;
    }
    if(!error_.empty()) return false;
    if(!width || !height) {
        error_ = "A fonte não enviou vídeo MPEG-PS compatível. Use a lista do conversor Cloud360.";
        return false;
    }
    if(width > 1280 || height > 720 || (width & 1) || (height & 1)) {
        error_ = "Dimensões de vídeo incompatíveis. Use a lista do conversor Cloud360.";
        return false;
    }
    plm_buffer_t* buffer = plm_buffer_create_with_capacity(128 * 1024);
    plm_buffer_write(buffer, &prefix[0], prefix.size());
    plm_buffer_set_load_callback(buffer, Load, this);
    decoder_ = plm_create_with_buffer(buffer, 1);
    if(!plm_has_headers(decoder_) || plm_get_width(decoder_) != (int)width ||
       plm_get_height(decoder_) != (int)height || plm_get_framerate(decoder_) <= 0 ||
       (plm_get_num_audio_streams(decoder_) && !SampleRate())) {
        if(error_.empty()) error_ = "Formato incompatível. Use vídeo MPEG-1 e áudio MP2.";
        return false;
    }
    plm_set_video_decode_callback(decoder_, Frame, this);
    plm_set_audio_decode_callback(decoder_, Samples, this);
    plm_set_audio_lead_time(decoder_, 0.075);
    return error_.empty();
}

void MpegPlayback::Load(plm_buffer_t* buffer, void* context) {
    MpegPlayback& self = *static_cast<MpegPlayback*>(context);
    if(!self.eof_ && self.error_.empty()) {
        if(self.budget_ < ReadSize) self.error_ = "A transmissão não entregou um quadro de mídia válido.";
        else {
            unsigned char bytes[ReadSize];
            int n = self.Fill(bytes, ReadSize);
            if(n) {
                plm_buffer_write(buffer, bytes, n);
                self.budget_ -= n;
            }
        }
    }
    if(self.eof_ || !self.error_.empty()) plm_buffer_signal_end(buffer);
}
void MpegPlayback::Frame(plm_t*, plm_frame_t* frame, void* context) {
    MpegPlayback& self = *static_cast<MpegPlayback*>(context);
    self.sawFrame_ = true;
    if(self.error_.empty() && !self.output_.Video(*frame)) self.error_ = "Falha na saída de vídeo.";
}
void MpegPlayback::Samples(plm_t*, plm_samples_t* samples, void* context) {
    MpegPlayback& self = *static_cast<MpegPlayback*>(context);
    if(self.error_.empty() && !self.output_.Audio(*samples)) self.error_ = "Falha na saída de áudio.";
}
int MpegPlayback::SampleRate() const { return decoder_ ? plm_get_samplerate(decoder_) : 0; }
double MpegPlayback::PixelAspect() const { return decoder_ ? plm_get_pixel_aspect_ratio(decoder_) : 1; }
bool MpegPlayback::Advance(double seconds) {
    if(!decoder_ || !error_.empty() || plm_has_ended(decoder_)) return false;
    budget_ = StepLimit;
    plm_decode(decoder_, seconds);
    if(plm_has_ended(decoder_) && !sawFrame_ && error_.empty()) error_ = "A fonte terminou sem entregar quadros de vídeo.";
    return error_.empty() && !plm_has_ended(decoder_);
}
