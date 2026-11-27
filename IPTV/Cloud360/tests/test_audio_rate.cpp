#include "audio_rate.h"
#include <cmath>
int main(){
    for(unsigned rate:{32000u,44100u,48000u}){
        CloudAudioRate a,b;
        std::vector<float> in(2000),full,part,joined;
        for(size_t i=0;i<in.size();++i)in[i]=float(i%97)/97;
        if(!a.Convert(in,rate,full))return 1;
        for(size_t i=0;i<in.size();i+=74){
            const auto end=i+74<in.size()?i+74:in.size();
            if(!b.Convert(std::vector<float>(in.begin()+i,in.begin()+end),rate,part))return 2;
            joined.insert(joined.end(),part.begin(),part.end());
        }
        if(joined!=full||full.size()!=2*(1000u*48000/rate))return 3;
        if(rate==48000&&full!=in)return 4;
        if(b.Convert(in,22050,part))return 5;
    }
    return 0;
}
