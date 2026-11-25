#include "container_decode.h"
#include "frame_output.h"
#include <vector>
#include <cstdio>
#include <cstdint>
struct Output {
    std::vector<unsigned char> rgb;
    std::vector<float> pcm;
    uint64_t frames=0,samples=0;
    bool cancel=false;
    static int Frame(void* user,const AVFrame* f,int video,int64_t pts){
        auto& out=*static_cast<Output*>(user);(void)pts;
        if(out.cancel)return -1;
        if(video){
            out.rgb.resize(static_cast<size_t>(f->width)*f->height*3);
            if(cloud_frame_rgb(f,out.rgb.data(),out.rgb.size()))return -1;
            ++out.frames;
        }else{
            out.pcm.resize(static_cast<size_t>(f->nb_samples)*2);
            if(cloud_frame_stereo(f,out.pcm.data(),out.pcm.size()))return -1;
            out.samples+=f->nb_samples;
        }
        return 0;
    }
};
int main(int argc,char** argv){
    if(argc!=2)return 2;
    Output out;
    auto* s=cloud_container_open(argv[1],Output::Frame,&out,1);
    if(!s)return 1;
    int rc=1;unsigned calls=0;
    if(cloud_container_step(s,0)>=0||cloud_container_step(s,65)>=0){cloud_container_close(s);return 1;}
    while(rc>0&&calls++<250000)rc=cloud_container_step(s,1);
    const auto report=*cloud_container_report(s);
    const bool ok=rc==0&&cloud_container_step(s,1)==0&&report.video_frames==out.frames&&
        report.audio_samples==out.samples&&out.frames>=50&&out.samples>48000*8&&calls>100;
    std::printf("steps=%u frames=%llu samples=%llu reusable_bytes=%zu\n",calls,(unsigned long long)out.frames,
        (unsigned long long)out.samples,out.rgb.capacity()+out.pcm.capacity()*sizeof(float));
    cloud_container_close(s);
    if(!ok)return 1;
    out.cancel=true;s=cloud_container_open(argv[1],Output::Frame,&out,1);if(!s)return 1;
    do{rc=cloud_container_step(s,4);}while(rc>0);
    const bool cancelled=rc<0&&cloud_container_step(s,1)==rc;
    cloud_container_close(s);
    return cancelled?0:1;
}
