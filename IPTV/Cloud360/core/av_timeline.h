#pragma once
#include "container_decode.h"
#include "frame_output.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <vector>
#include <utility>

// Short local clips only. Decode before opening output devices, reject excess
// memory/duration, then schedule both streams on the same monotonic clock.
class CloudAvTimeline {
public:
    struct Video { int64_t pts; unsigned width,height; double aspect; std::vector<unsigned char> rgb; };
    struct Audio { int64_t pts; unsigned samples; std::vector<float> pcm; };
    static constexpr size_t MaxBytes=32u*1024u*1024u;
    static constexpr int64_t MaxDuration=8000000,AudioLead=20000;
    static int Capture(void* opaque,const AVFrame* frame,int video,int64_t pts){
        return static_cast<CloudAvTimeline*>(opaque)->Add(frame,video!=0,pts)?0:-1;
    }
    bool Add(const AVFrame* frame,bool video,int64_t pts){
        if(prepared_||!frame||pts==std::numeric_limits<int64_t>::min()||
           pts < -86400000000LL || pts > 86400000000LL)return false;
        if(video){
            if(frame->width<1||frame->height<1||frame->width>1280||frame->height>720||
               (!videos_.empty()&&pts<videos_.back().pts)||videos_.size()>=240)return false;
            const size_t bytes=static_cast<size_t>(frame->width)*frame->height*3;
            if(bytes>MaxBytes-used_)return false;
            double aspect=1.0;
            if(frame->sample_aspect_ratio.num>0&&frame->sample_aspect_ratio.den>0)
                aspect=double(frame->sample_aspect_ratio.num)/frame->sample_aspect_ratio.den;
            Video v{pts,static_cast<unsigned>(frame->width),static_cast<unsigned>(frame->height),aspect,{}};
            v.rgb.resize(bytes);
            if(cloud_frame_rgb(frame,v.rgb.data(),bytes))return false;
            used_+=bytes;videos_.push_back(std::move(v));
        }else{
            if(frame->sample_rate!=48000||frame->nb_samples<1||frame->nb_samples>8192||
               (!audios_.empty()&&pts<audios_.back().pts)||audios_.size()>=512)return false;
            const size_t samples=static_cast<size_t>(frame->nb_samples)*2,bytes=samples*sizeof(float);
            if(bytes>MaxBytes-used_)return false;
            Audio a{pts,static_cast<unsigned>(frame->nb_samples),{}};
            a.pcm.resize(samples);
            if(cloud_frame_stereo(frame,a.pcm.data(),samples))return false;
            used_+=bytes;audios_.push_back(std::move(a));
        }
        return true;
    }
    bool Prepare(){
        if(prepared_||videos_.empty()||audios_.empty())return false;
        origin_=std::min(videos_.front().pts,audios_.front().pts);
        const int64_t end=std::max(videos_.back().pts+40000,
            audios_.back().pts+int64_t(audios_.back().samples)*1000000/48000);
        if(end<=origin_||end-origin_>MaxDuration)return false;
        // This diagnostic supports continuous 48 kHz AAC, not discontinuous
        // live streams. Reject material gaps/overlaps rather than silently drift.
        for(size_t i=1;i<audios_.size();++i){
            const auto& prev=audios_[i-1];
            const auto expected=prev.pts+int64_t(prev.samples)*1000000/48000;
            if(audios_[i].pts-expected>5000||expected-audios_[i].pts>5000)return false;
        }
        duration_=end-origin_;prepared_=true;return true;
    }
    template<class V,class A>
    bool Pump(int64_t elapsed,V video,A audio){
        if(!prepared_||elapsed<last_||elapsed<0)return false;
        last_=elapsed;
        // Enqueue audio ahead of the presentation clock in native 256-sample
        // blocks. Reject severe starvation instead of playing stale audio.
        unsigned sent=0;
        while(ai_<audios_.size()&&sent<16){
            const auto& a=audios_[ai_];
            const int64_t due=a.pts-origin_+int64_t(offset_)*1000000/48000;
            if(due>elapsed+AudioLead)break;
            if(elapsed-due>250000)return false;
            const unsigned count=std::min(256u,a.samples-offset_);
            if(!audio(a.pcm.data()+2*offset_,count))return false;
            ++sent;offset_+=count;
            if(offset_==a.samples){offset_=0;++ai_;}
        }
        size_t next=vi_;
        while(next<videos_.size()&&videos_[next].pts-origin_<=elapsed)++next;
        if(next>vi_){
            dropped_+=next-vi_-1;
            if(!video(videos_[next-1]))return false;
            vi_=next;
        }
        return true;
    }
    bool Finished(int64_t elapsed) const{return prepared_&&vi_==videos_.size()&&ai_==audios_.size()&&elapsed>=duration_;}
    size_t Bytes() const{return used_;}
    size_t Dropped() const{return dropped_;}
    int64_t Duration() const{return duration_;}
private:
    std::vector<Video> videos_;
    std::vector<Audio> audios_;
    size_t used_=0,vi_=0,ai_=0,dropped_=0;
    unsigned offset_=0;
    int64_t origin_=0,duration_=0,last_=0;
    bool prepared_=false;
};
