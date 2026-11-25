#include "container_decode.h"
#include "av_stream_queue.h"
#include "playback_controls.h"
#include <cstdio>
int main(int argc,char** argv){
    if(argc!=2)return 2;
    CloudAvStreamQueue queue;
    auto* s=cloud_container_open(argv[1],CloudAvStreamQueue::Capture,&queue,1);
    if(!s)return 1;
    auto prefill=[&](){int rc=1;unsigned n=0;while(rc>0&&queue.WantsInput(0)&&n++<4096)rc=cloud_container_step(s,1);if(!rc)queue.EndInput();return rc>=0&&queue.Prepare();};
    const auto duration=cloud_container_duration(s),start=cloud_container_start(s);
    bool ok=duration>=11000000&&duration<=13000000&&prefill();
    ok=ok&&cloud_container_seek(s,-1)<0&&cloud_container_seek(s,duration)<0;
    if(ok){ok=cloud_container_seek(s,6500000)==0;queue.Reset();ok=ok&&prefill()&&queue.Origin()-start>=3500000&&queue.Origin()-start<=6500000;}
    if(ok){ok=cloud_container_seek(s,0)==0;queue.Reset();ok=ok&&prefill()&&queue.Origin()-start<1000000;}
    cloud_container_close(s);
    using C=CloudPlaybackControls;C local(true),remote(false);
    local.Update(C::Forward,1);ok=ok&&local.TakeSeek()==1&&local.TakeSeek()==0;
    local.Update(C::Forward,2);ok=ok&&local.TakeSeek()==0;
    local.Update(C::Backward,3);ok=ok&&local.TakeSeek()==-1;
    remote.Update(C::Forward,1);ok=ok&&remote.TakeSeek()==0;
    std::printf("duration=%lld seek=%s\n",(long long)duration,ok?"PASS":"FAIL");return ok?0:1;
}
