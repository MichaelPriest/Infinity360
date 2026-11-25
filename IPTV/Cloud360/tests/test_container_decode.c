#include "container_decode.h"
#include <stdio.h>
#include <string.h>
int main(int argc,char** argv){
    CloudContainerReport r;
    if(argc!=2)return 2;
    int rc=cloud_container_decode(argv[1],&r);
    printf("rc=%d video=%u audio=%u samples=%llu dimensions=%ux%u\n",rc,r.video_frames,r.audio_frames,(unsigned long long)r.audio_samples,r.width,r.height);
    return rc||!r.video_frames||!r.audio_samples||r.width!=128||r.height!=72?1:0;
}
