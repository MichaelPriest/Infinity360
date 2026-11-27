#include "av_stream_queue.h"
#include <cstdio>
int main(int argc,char** argv){
    unsigned w=0,h=0;
    if(!cloud_video_fit(1920,1080,&w,&h)||w!=1280||h!=720||
       !cloud_video_fit(1440,1080,&w,&h)||w!=960||h!=720||
       cloud_video_fit(3840,2160,&w,&h)||cloud_video_fit(0,720,&w,&h))return 1;
    if(argc!=2)return 2;
    CloudAvStreamQueue queue;
    auto* s=cloud_container_open(argv[1],CloudAvStreamQueue::Capture,&queue,1);
    if(!s)return 3;
    int rc=1;unsigned calls=0;
    while(rc>0&&queue.WantsInput(0)&&calls++<4096)rc=cloud_container_step(s,1);
    if(rc==0)queue.EndInput();
    bool ok=rc>=0&&queue.Prepare();
    unsigned shown=0;uint64_t samples=0;int64_t elapsed=0;
    auto video=[&](const CloudAvStreamQueue::Video& v){
        ++shown;return v.width==1280&&v.height==720&&v.rgb.size()==1280u*720*3&&v.aspect>0.99&&v.aspect<1.01;
    };
    auto audio=[&](const float* pcm,unsigned n){samples+=n;return pcm&&n&&n<=256;};
    for(;ok&&elapsed<5000000&&!queue.Finished(elapsed);elapsed+=5000){
        unsigned budget=64;
        while(rc>0&&queue.WantsInput(elapsed)&&budget--)rc=cloud_container_step(s,1);
        if(rc==0)queue.EndInput();
        ok=rc>=0&&queue.Pump(elapsed,video,audio);
    }
    const auto report=*cloud_container_report(s);
    ok=ok&&queue.Finished(elapsed)&&shown>=8&&samples>=80000&&report.width==1920&&report.height==1080&&
       report.video_frames==shown+queue.Dropped()&&queue.Bytes()==0&&queue.Peak()<=CloudAvStreamQueue::MaxBytes;
    std::printf("1080 source %ux%u, 720 output frames=%u samples=%llu peak=%zu\n",report.width,report.height,shown,(unsigned long long)samples,queue.Peak());
    cloud_container_close(s);return ok?0:4;
}
