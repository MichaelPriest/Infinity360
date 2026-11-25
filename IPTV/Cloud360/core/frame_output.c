#include "frame_output.h"
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <math.h>
static unsigned char clip(int n){return n<0?0:n>255?255:(unsigned char)n;}
int cloud_frame_rgb(const AVFrame* f,unsigned char* dst,size_t bytes){
    if(!f||!dst||f->width<1||f->height<1||f->width>1280||f->height>720||
       (f->format!=AV_PIX_FMT_YUV420P&&f->format!=AV_PIX_FMT_YUVJ420P)||
       bytes<(size_t)f->width*f->height*3)return -1;
    if(!f->data[0]||!f->data[1]||!f->data[2]||f->linesize[0]<f->width||
       f->linesize[1]<(f->width+1)/2||f->linesize[2]<(f->width+1)/2)return -1;
    int full=f->color_range==AVCOL_RANGE_JPEG||f->format==AV_PIX_FMT_YUVJ420P;
    int hd=f->colorspace==AVCOL_SPC_BT709;
    for(int y=0;y<f->height;++y)for(int x=0;x<f->width;++x){
        int Y=f->data[0][y*f->linesize[0]+x],u=f->data[1][(y/2)*f->linesize[1]+x/2]-128,v=f->data[2][(y/2)*f->linesize[2]+x/2]-128;
        int c=full?Y*256:(Y-16)*298;
        int rv=full?(hd?403:359):(hd?459:409);
        int gu=full?(hd?48:88):(hd?55:100);
        int gv=full?(hd?120:183):(hd?136:208);
        int bu=full?(hd?475:454):(hd?541:516);
        size_t p=3*((size_t)y*f->width+x);
        dst[p]=clip((c+rv*v+128)/256);
        dst[p+1]=clip((c-gu*u-gv*v+128)/256);
        dst[p+2]=clip((c+bu*u+128)/256);
    }
    return 0;
}
int cloud_frame_stereo(const AVFrame* f,float* dst,size_t samples){
    if(!f||!dst||f->format!=AV_SAMPLE_FMT_FLTP||f->nb_samples<1||f->nb_samples>8192||
       samples<(size_t)f->nb_samples*2||!f->extended_data)return -1;
    int channels=f->ch_layout.nb_channels;
    if(channels<1||channels>2||!f->extended_data[0]||!f->extended_data[channels-1])return -1;
    const float* l=(const float*)f->extended_data[0],*r=(const float*)f->extended_data[channels-1];
    for(int i=0;i<f->nb_samples;++i){
        float a=l[i],b=r[i];
        dst[2*i]=!isfinite(a)?0:a<-1?-1:a>1?1:a;
        dst[2*i+1]=!isfinite(b)?0:b<-1?-1:b>1?1:b;
    }
    return 0;
}
