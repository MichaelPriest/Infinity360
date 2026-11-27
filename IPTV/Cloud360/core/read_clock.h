#pragma once
#include <algorithm>
#include <cstdint>

// Compensation for a synchronous decoder read that stalls the UI thread.
// Clock input is monotonic microseconds since playback start. No claim of
// device-level A/V synchronization: queued hardware audio cannot be recalled.
class CloudReadClock {
public:
    static constexpr int64_t MinAhead=150000,MaxAhead=600000;
    static constexpr int64_t SlowRead=50000,MaxStall=15000000,MaxTotal=900000000;
    bool Observe(int64_t before,int64_t after,int64_t bufferedUs=0){
        if(before<last_||after<before)return false;
        const int64_t duration=after-before;
        if(duration>MaxStall)return false;
        if(duration>SlowRead){
            // Only explicitly supplied, usable lead may absorb a read delay.
            const auto absorbed=std::min(duration,std::max<int64_t>(0,bufferedUs));
            const auto uncovered=duration-absorbed;
            if(uncovered>MaxTotal-offset_)return false;
            offset_+=uncovered;++stalls_;
            const auto increase=std::max<int64_t>(50000,std::min<int64_t>(150000,duration/4));
            ahead_=std::min(MaxAhead,ahead_+increase);fast_=0;
        }else if(++fast_>=64){
            ahead_=std::max(MinAhead,ahead_-25000);fast_=0;
        }
        last_=after;
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
