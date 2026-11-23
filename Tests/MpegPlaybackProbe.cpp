#include "MpegPlayback.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <vector>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

class Input : public MediaReader {
    int Read(unsigned char* data,unsigned capacity) {
        // Vary the size of compressed reads independently of frame boundaries.
        const size_t n=std::fread(data,1,capacity>1301?1301:capacity,stdin);
        return n ? static_cast<int>(n) : std::ferror(stdin) ? -1 : 0;
    }
};
class Output : public MpegOutput {
public:
    unsigned frames,changes,samples,nonzero,crossings,width,height,previous;
    bool below;
    std::vector<unsigned char> rgb;
    Output() : frames(0),changes(0),samples(0),nonzero(0),crossings(0),width(0),height(0),previous(0),below(false) {}
    bool Video(plm_frame_t& frame) {
        width=frame.width;height=frame.height;
        rgb.resize(width*height*3);
        plm_frame_to_rgb(&frame,&rgb[0],width*3);
        unsigned hash=2166136261u;
        for(size_t i=0;i<rgb.size();++i) hash=(hash^rgb[i])*16777619u;
        if(frames && previous!=hash) ++changes;
        if(!frames) std::cerr<<"frame"<<std::endl;
        previous=hash;++frames;return true;
    }
    bool Audio(const plm_samples_t& block) {
        short pcm[PLM_AUDIO_SAMPLES_PER_FRAME*2];
        mpegStereoPcm16(block,pcm);
        for(unsigned i=0;i<block.count;++i) {
            const short value=pcm[2*i];
            if(value>164 || value < -164) ++nonzero;
            if(value < -492) below=true;
            if(below && value>492) {++crossings;below=false;}
        }
        samples+=block.count;
        return true;
    }
};
int main() {
#ifdef _WIN32
    _setmode(_fileno(stdin),_O_BINARY);
#endif
    Input input;
    Output output;
    MpegPlayback player(input,output);
    if(!player.Open()) {std::cerr<<player.Error()<<std::endl;return 1;}
    for(unsigned i=0;player.Advance(0.020);++i) {
        if(i==3000) return 2;
    }
    if(!player.Error().empty()) {std::cerr<<player.Error()<<std::endl;return 1;}
    std::cout<<output.frames<<' '<<output.changes<<' '<<output.samples<<' '<<output.nonzero<<' '
             <<output.crossings<<' '<<player.SampleRate()<<' '<<output.width<<' '<<output.height<<std::endl;
    return 0;
}
