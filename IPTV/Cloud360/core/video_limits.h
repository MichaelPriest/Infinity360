#ifndef CLOUD360_VIDEO_LIMITS_H
#define CLOUD360_VIDEO_LIMITS_H
/* Visible source resolution; decoder allocation permits aligned 1088 rows.
 * RGB presentation and the existing queue remain limited to 1280x720. */
#define CLOUD_VIDEO_INPUT_W 1920
#define CLOUD_VIDEO_INPUT_H 1080
#define CLOUD_VIDEO_DECODE_PIXELS (1920LL*1088)
#define CLOUD_VIDEO_OUTPUT_W 1280
#define CLOUD_VIDEO_OUTPUT_H 720
static inline int cloud_video_fit(int w,int h,unsigned* ow,unsigned* oh){
    if(!ow||!oh||w<=0||h<=0||w>CLOUD_VIDEO_INPUT_W||h>CLOUD_VIDEO_INPUT_H)return 0;
    if(w<=CLOUD_VIDEO_OUTPUT_W&&h<=CLOUD_VIDEO_OUTPUT_H){*ow=(unsigned)w;*oh=(unsigned)h;return 1;}
    if(w*CLOUD_VIDEO_OUTPUT_H>=h*CLOUD_VIDEO_OUTPUT_W){
        *ow=CLOUD_VIDEO_OUTPUT_W;*oh=(unsigned)(h*CLOUD_VIDEO_OUTPUT_W/w);
    }else{
        *oh=CLOUD_VIDEO_OUTPUT_H;*ow=(unsigned)(w*CLOUD_VIDEO_OUTPUT_H/h);
    }
    return *ow&&*oh;
}
#endif
