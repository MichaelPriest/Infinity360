#pragma once
#include "av_timeline.h"
#include <deque>
#include "audio_rate.h"

// Single-threaded bounded local playback. Read ahead by packet, not whole file.
// Fail closed if an input has excessive interleave skew, discontinuous audio,
// missing timestamps or decoder output exceeding the hard queue budget.
class CloudAvStreamQueue {
public:
    using Video=CloudAvTimeline::Video;
    using Audio=CloudAvTimeline::Audio;
    enum class PumpFailure { None, Clock, AudioLate, AudioSink, VideoSink };
    PumpFailure LastPumpFailure()const{return pumpFailure_;}
    // Software queue only: excludes samples already submitted to the audio device.
    int64_t BufferedAhead(int64_t elapsed)const{
        if(!ready_||elapsed<0||video_.empty()||(audio_.empty()&&!pcmCount_))return 0;
        return std::max<int64_t>(0,std::min(lastVideo_+videoPeriod_,audioEnd_)-origin_-elapsed);
    }
    static constexpr size_t MaxBytes=32u*1024u*1024u;
    static constexpr int64_t AudioLead=20000, MaxDuration=21600000000LL;
    static int Capture(void* user,const AVFrame* frame,int video,int64_t pts){
        return static_cast<CloudAvStreamQueue*>(user)->Add(frame,video!=0,pts)?0:-1;
    }
    bool Add(const AVFrame* f,bool video,int64_t pts){
        if(!f||pts < -86400000000LL||pts>86400000000LL||eof_)return false;
        if(ready_&&(pts<origin_||pts-origin_>MaxDuration))return false;
        if(video){
            unsigned width=0,height=0;
            if(!cloud_video_fit(f->width,f->height,&width,&height)||
                (haveVideo_&&pts<lastVideo_)||video_.size()>=64)return false;
            size_t bytes=static_cast<size_t>(width)*height*3;
            if(bytes>MaxBytes-used_)return false;
            double aspect=1.0;
            if(f->sample_aspect_ratio.num>0&&f->sample_aspect_ratio.den>0)
                aspect=double(f->sample_aspect_ratio.num)/f->sample_aspect_ratio.den;
            // Retain display aspect even when integer scaling rounds one dimension.
            aspect*=double(f->width)*height/(double(f->height)*width);
            Video v{pts,width,height,aspect,{}};
            v.rgb.resize(bytes);
            if(cloud_frame_rgb_scaled(f,v.rgb.data(),bytes,width,height))return false;
            if(haveVideo_&&pts>lastVideo_)videoPeriod_=std::min<int64_t>(1000000,pts-lastVideo_);
            lastVideo_=pts;haveVideo_=true;used_+=bytes;video_.push_back(std::move(v));
        }else{
            // Native sink is 48 kHz and submits complete 256-sample frames.
            if(!CloudAudioRate::Supported(f->sample_rate)||f->nb_samples<1||f->nb_samples>8192||
                audio_.size()>=256)return false;
            if(haveAudio_&&(pts<lastAudio_||pts-audioEnd_>5000||audioEnd_-pts>5000))return false;
            const size_t n=static_cast<size_t>(f->nb_samples)*2;
            std::vector<float> source(n);
            if(cloud_frame_stereo(f,source.data(),n))return false;
            Audio a{pts,0,{}};
            if(!rate_.Convert(source,static_cast<unsigned>(f->sample_rate),a.pcm))return false;
            a.samples=static_cast<unsigned>(a.pcm.size()/2);
            const size_t bytes=a.pcm.size()*sizeof(float);
            if(!a.samples||bytes>MaxBytes-used_)return false;
            // Source PTS remains authoritative; converted samples retain fractional phase.
            lastAudio_=pts;audioEnd_=pts+int64_t(a.samples)*1000000/48000;
            haveAudio_=true;used_+=bytes;audio_.push_back(std::move(a));
        }
        peak_=std::max(peak_,used_);return true;
    }
    bool Ready() const{return ready_;}
    int64_t Origin() const{return origin_;}
    void Reset(){*this=CloudAvStreamQueue{};}
    bool Prepare(){
        if(ready_||video_.empty()||audio_.empty())return false;
        origin_=std::min(video_.front().pts,audio_.front().pts);
        if(std::max(lastVideo_,audioEnd_)-origin_>MaxDuration)return false;
        ready_=true;return true;
    }
    bool WantsInput(int64_t elapsed,int64_t ahead=150000) const{
        ahead=std::max<int64_t>(150000,std::min<int64_t>(600000,ahead));
        if(eof_)return false;
        // Leave headroom for a packet that produces multiple frames; hard
        // cap is enforced separately on each callback.
        if(used_>=MaxBytes*3/4)return false;
        if(!ready_)return video_.size()<2||audio_.empty()||audioEnd_-audio_.front().pts<100000;
        return video_.empty()||audio_.empty()||lastVideo_-origin_<elapsed+ahead||audioEnd_-origin_<elapsed+ahead;
    }
    void EndInput(){eof_=true;}
    template<class V,class A>
    bool Pump(int64_t elapsed,V video,A audio){
        pumpFailure_=PumpFailure::None;
        if(!ready_||elapsed<lastClock_||elapsed<0){pumpFailure_=PumpFailure::Clock;return false;}
        lastClock_=elapsed;
        unsigned sent=0;
        // Keep partial audio samples across decoder frames. Only pad the final
        // hardware block at EOF; do not insert silence between AAC frames.
        while(sent<16){
            if(!pcmCount_){
                if(audio_.empty())break;
                pcmPts_=audio_.front().pts+int64_t(offset_)*1000000/48000-origin_;
            }
            if(pcmPts_>elapsed+AudioLead)break;
            if(elapsed-pcmPts_>250000){pumpFailure_=PumpFailure::AudioLate;return false;}
            while(pcmCount_<256&&!audio_.empty()){
                auto& a=audio_.front();
                unsigned count=std::min(256u-pcmCount_,a.samples-offset_);
                std::copy_n(a.pcm.data()+2*offset_,2*count,pcm_+2*pcmCount_);
                pcmCount_+=count;offset_+=count;
                if(offset_==a.samples){
                    used_-=a.pcm.size()*sizeof(float);audio_.pop_front();offset_=0;
                }
            }
            if(pcmCount_<256&&!eof_)break;
            if(!pcmCount_)break;
            if(!audio(pcm_,pcmCount_)){pumpFailure_=PumpFailure::AudioSink;return false;}
            pcmCount_=0;++sent;
        }
        while(video_.size()>1&&video_[1].pts-origin_<=elapsed){
            used_-=video_.front().rgb.size();video_.pop_front();++dropped_;
        }
        if(!video_.empty()&&video_.front().pts-origin_<=elapsed){
            if(!video(video_.front())){pumpFailure_=PumpFailure::VideoSink;return false;}
            used_-=video_.front().rgb.size();video_.pop_front();
        }
        return true;
    }
    bool Finished(int64_t elapsed)const{
        return eof_&&ready_&&video_.empty()&&audio_.empty()&&!pcmCount_&&
            elapsed>=std::max(lastVideo_+videoPeriod_,audioEnd_)-origin_;
    }
    size_t Bytes()const{return used_;}
    size_t Peak()const{return peak_;}
    size_t Dropped()const{return dropped_;}
private:
    PumpFailure pumpFailure_=PumpFailure::None;
    CloudAudioRate rate_;
    std::deque<Video> video_;
    std::deque<Audio> audio_;
    float pcm_[512]{};
    unsigned pcmCount_=0,offset_=0;
    size_t used_=0,peak_=0,dropped_=0;
    int64_t origin_=0,lastClock_=0,lastVideo_=0,lastAudio_=0,audioEnd_=0,pcmPts_=0,videoPeriod_=40000;
    bool ready_=false,eof_=false,haveVideo_=false,haveAudio_=false;
};
