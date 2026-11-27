#include "av_stream_queue.h"
#include <cstdio>
int main(int argc,char** argv){
    if(argc!=2)return 2;
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
    bool ok=true;
    for(;elapsed<20000000&&!queue.Finished(elapsed);elapsed+=5000){
        unsigned budget=64;
        while(rc>0&&queue.WantsInput(elapsed)&&budget--)rc=cloud_container_step(s,1);
        if(rc==0)queue.EndInput();
        if(rc<0||!queue.Pump(elapsed,video,audio)){ok=false;break;}
    }
    const auto report=*cloud_container_report(s);
    ok=ok&&queue.Finished(elapsed)&&elapsed>8000000&&report.video_frames==shown+queue.Dropped()&&
        report.audio_samples==samples&&queue.Bytes()==0&&queue.Peak()<4*1024*1024;
    std::printf("shown=%u dropped=%zu samples=%llu peak=%zu elapsed=%lld\n",shown,queue.Dropped(),
        (unsigned long long)samples,queue.Peak(),(long long)elapsed);
    cloud_container_close(s);return ok?0:1;
}
