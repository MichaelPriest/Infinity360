#pragma once
#include <algorithm>
#include <cstdint>

// Input edges and pause accounting independent of XSDK. Wall time is
// monotonic microseconds; pause is available only for local media.
class CloudPlaybackControls {
public:
    enum Key : unsigned { Pause=1, Mute=2, Quieter=4, Louder=8, Backward=16, Forward=32 };
    explicit CloudPlaybackControls(bool local,unsigned held=0):previous_(held),local_(local){}
    bool Update(unsigned keys,int64_t wall){
        const unsigned edges=keys&~previous_;previous_=keys;
        if(local_&&!paused_){if(edges&Backward)seek_=-1;else if(edges&Forward)seek_=1;}
        bool changed=false;
        if(local_&&(edges&Pause)){
            if(paused_){
                if(wall<pausedAt_)return false;
                pausedTotal_+=wall-pausedAt_;
            }else pausedAt_=wall;
            paused_=!paused_;changed=true;
        }
        if(edges&Mute){muted_=!muted_;changed=true;}
        const int old=volume_;
        if(edges&Quieter)volume_=std::max(0,volume_-10);
        if(edges&Louder)volume_=std::min(100,volume_+10);
        return changed||old!=volume_;
    }
    int64_t Clock(int64_t wall)const{
        return std::max<int64_t>(0,(paused_?pausedAt_:wall)-pausedTotal_);
    }
    int TakeSeek(){int result=seek_;seek_=0;return result;}
    void ResetClock(){paused_=false;pausedAt_=pausedTotal_=0;}
    bool Paused()const{return paused_;}
    bool Muted()const{return muted_;}
    int Volume()const{return volume_;}
    float Gain()const{return muted_?0.0f:volume_/100.0f;}
private:
    unsigned previous_=0;bool local_,paused_=false,muted_=false;
    int volume_=100,seek_=0;
    int64_t pausedAt_=0,pausedTotal_=0;
};
