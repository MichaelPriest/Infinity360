#pragma once
#include <vector>
#include <cstdint>
#include <cstddef>
// Causal first-order interpolation. Output count is floor(total input * 48000 / rate).
// One input-sample history preserves continuity across decoder frames.
class CloudAudioRate {
public:
    static bool Supported(int rate){return rate==32000||rate==44100||rate==48000;}
    bool Convert(const std::vector<float>& in,unsigned rate,std::vector<float>& out){
        out.clear();
        if(!Supported(rate)||in.empty()||in.size()%2||in.size()>16384||
            (rate_&&rate_!=rate))return false;
        rate_=rate;
        // Exact bypass avoids rounding error for already-native PCM.
        if(rate==48000){out=in;return true;}
        out.reserve((in.size()/2*48000/rate+2)*2);
        for(size_t i=0;i<in.size();i+=2){
            const float l=in[i],r=in[i+1];
            if(!have_){previousL_=l;previousR_=r;have_=true;}
            phase_+=48000;
            while(phase_>=rate){
                phase_-=rate;
                const float t=1.0f-static_cast<float>(phase_)/48000.0f;
                out.push_back(previousL_+(l-previousL_)*t);
                out.push_back(previousR_+(r-previousR_)*t);
            }
            previousL_=l;previousR_=r;
        }
        return true;
    }
private:
    unsigned rate_=0,phase_=0;
    float previousL_=0,previousR_=0;
    bool have_=false;
};
