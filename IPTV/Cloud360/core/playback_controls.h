#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
#include <cstddef>

// Input edges and pause accounting independent of XSDK. Wall time is
// monotonic microseconds; pause is available only for local media.
class CloudPlaybackControls {
public:
    enum Key : unsigned { Pause=1, Mute=2, Quieter=4, Louder=8, Backward=16, Forward=32 };
    struct AudioPreference {int volume=100;bool muted=false;};
    explicit CloudPlaybackControls(bool local,unsigned held=0):previous_(held),local_(local){}
    void RestoreAudio(const AudioPreference& p){volume_=std::max(0,std::min(100,p.volume));muted_=p.muted;}
    AudioPreference Audio()const{return AudioPreference{volume_,muted_};}
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
    void SynchronizeKeys(unsigned keys){previous_=keys;seek_=0;}
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

class CloudQualityMenu {
public:
    bool Open(const std::vector<unsigned>& caps,unsigned current){
        Close();options_.push_back(0);
        for(auto cap:caps)if(cap)options_.push_back(cap);
        std::sort(options_.begin(),options_.end());
        options_.erase(std::unique(options_.begin(),options_.end()),options_.end());
        if(options_.size()<3){Close();return false;}
        auto it=std::lower_bound(options_.begin(),options_.end(),current);
        index_=it!=options_.end()&&*it==current?static_cast<std::size_t>(it-options_.begin()):0;
        active_=true;return true;
    }
    void Close(){active_=false;index_=0;options_.clear();}
    bool Active()const{return active_;}
    void Move(int delta){
        if(!active_)return;
        if(delta<0&&index_>0)--index_;
        if(delta>0&&index_+1<options_.size())++index_;
    }
    unsigned Choice()const{return active_?options_[index_]:0;}
    std::size_t Index()const{return index_;}
    std::size_t Count()const{return options_.size();}
    bool Confirm(unsigned& current){
        if(!active_)return false;
        const unsigned selected=Choice();Close();
        if(selected==current)return false;
        current=selected;return true;
    }
private:
    std::vector<unsigned> options_;
    std::size_t index_=0;
    bool active_=false;
};
