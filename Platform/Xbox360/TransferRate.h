#pragma once
#include <chrono>
#include <string>
#include <cstdint>
#include "StorageInfo.h"

// Estimates only from observed byte deltas and monotonic time.
// Initial bytes of a resumed file do not count as new throughput.
class TransferRate {
public:
    static uint64_t Now(){
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count());
    }
    void Reset(){started_=false;rate_=0;last_=0;bytes_=0;total_=0;}
    bool Sample(unsigned long long done,unsigned long long total,uint64_t now){
        if(!started_||total!=total_||done<bytes_||now<last_){
            Reset();started_=true;last_=now;bytes_=done;total_=total;return true;
        }
        const auto elapsed=now-last_;
        if(elapsed<1000)return false;
        const auto delta=done-bytes_;
        // Recent intervals; no stale average after a stall or long gap.
        const double instant=static_cast<double>(delta)*1000.0/static_cast<double>(elapsed);
        rate_=!delta||elapsed>5000||rate_<=0?instant:0.65*rate_+0.35*instant;
        last_=now;bytes_=done;return true;
    }
    unsigned long long BytesPerSecond() const {
        return rate_>=1.0?static_cast<unsigned long long>(rate_):0;
    }
    unsigned long long RemainingSeconds(unsigned long long done,unsigned long long total) const {
        if(!total||done>=total||!BytesPerSecond())return 0;
        const auto remaining=total-done,speed=BytesPerSecond();
        return remaining/speed+(remaining%speed!=0);
    }
    std::string Label(unsigned long long done,unsigned long long total) const {
        if(total&&done>=total)return "Dados transferidos";
        const auto speed=BytesPerSecond();
        if(!speed)return "Velocidade e tempo: calculando...";
        const auto eta=RemainingSeconds(done,total);
        std::string label=StorageInfo::FormatBytes(speed)+"/s (estimativa)";
        if(!eta)return label;
        if(eta>24*3600)return label+" | >24h";
        return label+" | Restam ~"+(eta>=3600?std::to_string(eta/3600)+"h "+std::to_string((eta%3600)/60)+"min":eta>=60?std::to_string(eta/60)+"min "+std::to_string(eta%60)+"s":std::to_string(eta)+"s");
    }
private:
    bool started_=false;
    uint64_t last_=0;
    unsigned long long bytes_=0,total_=0;
    double rate_=0;
};
