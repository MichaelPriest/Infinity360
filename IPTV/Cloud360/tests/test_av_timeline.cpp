#include "av_timeline.h"
#include <cstdio>
int main(int argc,char** argv){
    if(argc!=2)return 2;
    CloudAvTimeline timeline;
    CloudContainerReport report{};
    if(cloud_container_decode_output(argv[1],&report,CloudAvTimeline::Capture,&timeline)||!timeline.Prepare())return 1;
    unsigned frames=0;uint64_t samples=0;int64_t time=0;
    auto video=[&](const CloudAvTimeline::Video& v){++frames;return v.width==128&&v.height==72&&!v.rgb.empty();};
    auto audio=[&](const float* pcm,unsigned n){samples+=n;return pcm&&n&&n<=256;};
    for(;time<=CloudAvTimeline::MaxDuration&&!timeline.Finished(time);time+=5000)
        if(!timeline.Pump(time,video,audio))return 1;
    if(!timeline.Finished(time)||samples!=report.audio_samples||frames+timeline.Dropped()!=report.video_frames||!frames)return 1;
    // A reverse clock is rejected rather than replaying queued data.
    if(timeline.Pump(0,video,audio))return 1;
    std::printf("frames=%u samples=%llu bytes=%zu duration_us=%lld dropped=%zu\n",frames,(unsigned long long)samples,timeline.Bytes(),(long long)timeline.Duration(),timeline.Dropped());
    CloudAvTimeline delayed;
    if(cloud_container_decode_output(argv[1],&report,CloudAvTimeline::Capture,&delayed)||!delayed.Prepare())return 1;
    if(delayed.Pump(500000,video,audio))return 1; // stale audio refused
    return 0;
}
