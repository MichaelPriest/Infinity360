#pragma once
#include <algorithm>
#include <cstdint>

// Compensation for a synchronous decoder read that stalls the UI thread.
// Clock input is monotonic microseconds since playback start. No claim of
// device-level A/V synchronization: queued hardware audio cannot be recalled.
class CloudReadClock {
public:
    static constexpr int64_t MinAhead=150000,MaxAhead=600000;
    static constexpr int64_t SlowRead=50000,MaxStall=15000000,MaxTotal=120000000;
    bool Observe(int64_t before,int64_t after){
        if(before<last_||after<before)return false;
        last_=after;
        const int64_t duration=after-before;
        if(duration>MaxStall)return false;
        if(duration>SlowRead){
            if(duration>MaxTotal-offset_)return false;
            offset_+=duration;++stalls_;
            ahead_=std::min(MaxAhead,ahead_+50000);fast_=0;
        }else if(++fast_>=64){
            ahead_=std::max(MinAhead,ahead_-25000);fast_=0;
        }
        return true;
    }
    int64_t Elapsed(int64_t wall)const{return std::max<int64_t>(0,wall-offset_);}
    int64_t Ahead()const{return ahead_;}
    int64_t Stalled()const{return offset_;}
    unsigned Stalls()const{return stalls_;}
private:
    int64_t last_=0,offset_=0,ahead_=MinAhead;
    unsigned stalls_=0,fast_=0;
};
