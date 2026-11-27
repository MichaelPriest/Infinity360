#include "av_stream_queue.h"
#include <cstdio>
int main(int argc,char** argv){
    if(argc!=2&&argc!=3)return 2;
    const unsigned sourceRate=argc==3?44100:48000;
    static_assert(CloudAvStreamQueue::MaxDuration==21600000000LL,"six hour timeline");
    static_assert(CloudAvStreamQueue::MaxBytes==32u*1024u*1024u,"unchanged memory ceiling");
    CloudAvStreamQueue queue;
    auto* s=cloud_container_open(argv[1],CloudAvStreamQueue::Capture,&queue,1);
    if(!s)return 1;
    int rc=1;
    unsigned calls=0;
    while(rc>0&&queue.WantsInput(0)&&calls++<4096)rc=cloud_container_step(s,1);
    if(rc==0)queue.EndInput();
    if(rc<0||!queue.Prepare()){cloud_container_close(s);return 1;}
    unsigned shown=0;uint64_t samples=0;
    auto video=[&](const CloudAvStreamQueue::Video& v){++shown;return v.width==128&&v.height==72;};
    auto audio=[&](const float* pcm,unsigned n){samples+=n;return pcm&&n&&n<=256;};
    int64_t elapsed=0;
    bool ok=!queue.Pump(-1,video,audio)&&queue.LastPumpFailure()==CloudAvStreamQueue::PumpFailure::Clock&&queue.BufferedAhead(-1)==0;
    for(;elapsed<20000000&&!queue.Finished(elapsed);elapsed+=5000){
        unsigned budget=64;
        while(rc>0&&queue.WantsInput(elapsed)&&budget--)rc=cloud_container_step(s,1);
        if(rc==0)queue.EndInput();
        if(rc<0||!queue.Pump(elapsed,video,audio)||queue.LastPumpFailure()!=CloudAvStreamQueue::PumpFailure::None){ok=false;break;}
    }
    const auto report=*cloud_container_report(s);
    ok=ok&&queue.Finished(elapsed)&&elapsed>8000000&&report.video_frames==shown+queue.Dropped()&&
        (report.audio_samples*48000/sourceRate==samples||report.audio_samples*48000/sourceRate+1==samples)&&queue.Bytes()==0&&queue.Peak()<4*1024*1024;
    std::printf("shown=%u dropped=%zu samples=%llu peak=%zu elapsed=%lld\n",shown,queue.Dropped(),
        (unsigned long long)samples,queue.Peak(),(long long)elapsed);
    cloud_container_close(s);
    // Fresh decoder: a deliberately late first pump must classify audio lateness
    // rather than call the output sink with stale samples.
    CloudAvStreamQueue late;
    auto* probe=cloud_container_open(argv[1],CloudAvStreamQueue::Capture,&late,1);
    if(!probe)return 1;
    rc=1;calls=0;
    while(rc>0&&late.WantsInput(0)&&calls++<4096)rc=cloud_container_step(probe,1);
    if(rc==0)late.EndInput();
    if(rc<0||!late.Prepare())ok=false;
    else{
        unsigned writes=0;
        auto sink=[&](const float*,unsigned){++writes;return true;};
        auto ignoreVideo=[](const CloudAvStreamQueue::Video&){return true;};
        ok=ok&&late.BufferedAhead(0)>0&&!late.Pump(1000000,ignoreVideo,sink)&&
            late.LastPumpFailure()==CloudAvStreamQueue::PumpFailure::AudioLate&&!writes;
    }
    cloud_container_close(probe);return ok?0:1;
}
