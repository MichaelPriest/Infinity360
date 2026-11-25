#include "container_decode.h"
#include <stdio.h>
int main(void){
    const char* paths[]={"game:/test.mp4","game:/test.ts"};
    for(unsigned i=0;i<2;++i){
        CloudContainerReport r={0};
        int rc=cloud_container_decode(paths[i],&r);
        printf("Cloud360 %s rc=%d video=%u samples=%llu dimensions=%ux%u\n",paths[i],rc,r.video_frames,(unsigned long long)r.audio_samples,r.width,r.height);
        if(rc)return 1;
    }
    return 0;
}
