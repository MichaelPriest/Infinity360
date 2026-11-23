#pragma once
#include <cstddef>
#include <string>
#define PLM_NO_STDIO
#include "third_party/pl_mpeg/pl_mpeg.h"

// The destination must hold samples.count stereo pairs in native byte order.
void mpegStereoPcm16(const plm_samples_t& samples, short* destination);

// A positive result is data, zero is end of stream, negative is a read error.
class MediaReader {
public:
    virtual ~MediaReader() {}
    virtual int Read(unsigned char* data, unsigned capacity)=0;
};

class MpegOutput {
public:
    virtual ~MpegOutput() {}
    virtual bool Video(plm_frame_t& frame)=0;
    virtual bool Audio(const plm_samples_t& samples)=0;
};

class MpegPlayback {
public:
    MpegPlayback(MediaReader& reader, MpegOutput& output);
    ~MpegPlayback();
    bool Open();
    bool Advance(double seconds); // false at EOF or on error
    const std::string& Error() const { return error_; }
    int SampleRate() const;
    double PixelAspect() const;
private:
    MpegPlayback(const MpegPlayback&);
    MpegPlayback& operator=(const MpegPlayback&);
    static void Load(plm_buffer_t* buffer, void* context);
    static void Frame(plm_t*, plm_frame_t* frame, void* context);
    static void Samples(plm_t*, plm_samples_t* samples, void* context);
    int Fill(unsigned char* bytes, unsigned size);
    MediaReader& reader_;
    MpegOutput& output_;
    plm_t* decoder_;
    std::string error_;
    unsigned budget_;
    bool eof_, sawFrame_;
};
