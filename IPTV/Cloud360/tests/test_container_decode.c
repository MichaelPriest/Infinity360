#include "container_decode.h"
#include "frame_output.h"
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct { unsigned frames,blocks; int nonzero,changed; unsigned previous; } Stats;
static int observe(void* user,const AVFrame* frame,int video,int64_t pts){
    (void)pts;Stats* s=user;
    if(video){
        unsigned char rgb[128*72*3];
        if(frame->width!=128||frame->height!=72||cloud_frame_rgb(frame,rgb,sizeof rgb))return -1;
        unsigned hash=2166136261u;
        for(size_t i=0;i<sizeof rgb;++i)hash=(hash^rgb[i])*16777619u;
        if(s->frames&&hash!=s->previous)s->changed=1;
        s->previous=hash;++s->frames;
    }else{
        float pcm[8192*2];
        if(cloud_frame_stereo(frame,pcm,8192*2))return -1;
        for(int i=0;i<frame->nb_samples*2;++i)if(fabsf(pcm[i])>0.01f)s->nonzero=1;
        ++s->blocks;
    }
    return 0;
}
static int cancel(void* user,const AVFrame* f,int video,int64_t pts){(void)user;(void)f;(void)video;(void)pts;return -1;}
int main(int argc,char** argv){
    CloudContainerReport r;
    if(argc!=2)return 2;
    Stats stats={0};
    int rc=cloud_container_decode_output(argv[1],&r,observe,&stats);
    printf("rc=%d video=%u audio=%u samples=%llu dimensions=%ux%u\n",rc,r.video_frames,r.audio_frames,(unsigned long long)r.audio_samples,r.width,r.height);
    if(rc||!r.video_frames||!r.audio_samples||r.width!=128||r.height!=72||!stats.changed||!stats.nonzero||stats.frames!=r.video_frames||stats.blocks!=r.audio_frames)return 1;
    return cloud_container_decode_output(argv[1],&r,cancel,NULL)==0?1:0;
}
