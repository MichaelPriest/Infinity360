#include "frame_output.h"
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/channel_layout.h>
#include <math.h>
#include <stdint.h>
#include <string.h>
static unsigned char clip(int n){return n<0?0:n>255?255:(unsigned char)n;}
int cloud_frame_rgb(const AVFrame* f,unsigned char* dst,size_t bytes){
    if(!f||f->width<1||f->height<1||f->width>CLOUD_VIDEO_OUTPUT_W||f->height>CLOUD_VIDEO_OUTPUT_H)return -1;
    return cloud_frame_rgb_scaled(f,dst,bytes,(unsigned)f->width,(unsigned)f->height);
}
int cloud_frame_rgb_scaled(const AVFrame* f,unsigned char* dst,size_t bytes,unsigned width,unsigned height){
    if(!f||!dst||f->width<1||f->height<1||f->width>CLOUD_VIDEO_INPUT_W||f->height>CLOUD_VIDEO_INPUT_H||
       !width||!height||width>CLOUD_VIDEO_OUTPUT_W||height>CLOUD_VIDEO_OUTPUT_H||
       width>(unsigned)f->width||height>(unsigned)f->height||
       (f->format!=AV_PIX_FMT_YUV420P&&f->format!=AV_PIX_FMT_YUVJ420P)||
       bytes<(size_t)width*height*3)return -1;
    if(!f->data[0]||!f->data[1]||!f->data[2]||f->linesize[0]<f->width||
       f->linesize[1]<(f->width+1)/2||f->linesize[2]<(f->width+1)/2)return -1;
    int full=f->color_range==AVCOL_RANGE_JPEG||f->format==AV_PIX_FMT_YUVJ420P;
    int hd=f->colorspace==AVCOL_SPC_BT709;
    // Coordinate table avoids a division per output pixel; memory <= 5 KiB.
    unsigned sx[CLOUD_VIDEO_OUTPUT_W];
    for(unsigned x=0;x<width;++x)sx[x]=x*(unsigned)f->width/width;
    for(unsigned y=0;y<height;++y){
        const unsigned sy=y*(unsigned)f->height/height;
        const uint8_t* yr=f->data[0]+sy*f->linesize[0];
        const uint8_t* ur=f->data[1]+(sy/2)*f->linesize[1];
        const uint8_t* vr=f->data[2]+(sy/2)*f->linesize[2];
        for(unsigned x=0;x<width;++x){
        int Y=yr[sx[x]],u=ur[sx[x]/2]-128,v=vr[sx[x]/2]-128;
        int c=full?Y*256:(Y-16)*298;
        int rv=full?(hd?403:359):(hd?459:409);
        int gu=full?(hd?48:88):(hd?55:100);
        int gv=full?(hd?120:183):(hd?136:208);
        int bu=full?(hd?475:454):(hd?541:516);
        size_t p=3*((size_t)y*width+x);
        dst[p]=clip((c+rv*v+128)/256);
        dst[p+1]=clip((c-gu*u-gv*v+128)/256);
        dst[p+2]=clip((c+bu*u+128)/256);
        }
    }
    return 0;
}
static float sample_at(const uint8_t* p,int format,size_t i){
    if(format==AV_SAMPLE_FMT_FLT){float v;memcpy(&v,p+i*sizeof v,sizeof v);return v;}
    if(format==AV_SAMPLE_FMT_S16){int16_t v;memcpy(&v,p+i*sizeof v,sizeof v);return (float)v/32768.0f;}
    if(format==AV_SAMPLE_FMT_S32){int32_t v;memcpy(&v,p+i*sizeof v,sizeof v);return (float)((double)v/2147483648.0);}
    return 0;
}
int cloud_frame_stereo(const AVFrame* f,float* dst,size_t samples){
    if(!f||!dst||f->nb_samples<1||f->nb_samples>8192||
       samples<(size_t)f->nb_samples*2||!f->extended_data)return -1;
    const enum AVSampleFormat fmt=(enum AVSampleFormat)f->format;
    const enum AVSampleFormat packed=av_get_packed_sample_fmt(fmt);
    if(packed!=AV_SAMPLE_FMT_FLT&&packed!=AV_SAMPLE_FMT_S16&&packed!=AV_SAMPLE_FMT_S32)return -1;
    const int channels=f->ch_layout.nb_channels,planar=av_sample_fmt_is_planar(fmt);
    if(channels<1||channels>8||!f->extended_data[0])return -1;
    for(int ch=0;ch<(planar?channels:1);++ch)if(!f->extended_data[ch])return -1;
    float wl[8]={0},wr[8]={0},sl=0,sr=0;
    if(channels==1)wl[0]=wr[0]=1;
    else if(channels==2){wl[0]=1;wr[1]=1;}
    else{
        if(!av_channel_layout_check(&f->ch_layout)||f->ch_layout.order==AV_CHANNEL_ORDER_UNSPEC)return -1;
        for(int ch=0;ch<channels;++ch){
            switch(av_channel_layout_channel_from_index(&f->ch_layout,ch)){
                case AV_CHAN_FRONT_LEFT:wl[ch]=1;break;
                case AV_CHAN_FRONT_RIGHT:wr[ch]=1;break;
                case AV_CHAN_FRONT_CENTER:wl[ch]=wr[ch]=0.70710678f;break;
                case AV_CHAN_BACK_LEFT:case AV_CHAN_SIDE_LEFT:wl[ch]=0.70710678f;break;
                case AV_CHAN_BACK_RIGHT:case AV_CHAN_SIDE_RIGHT:wr[ch]=0.70710678f;break;
                case AV_CHAN_BACK_CENTER:wl[ch]=wr[ch]=0.5f;break;
                case AV_CHAN_LOW_FREQUENCY:break; // no bass management in stereo sink
                default:return -1;
            }
        }
    }
    for(int ch=0;ch<channels;++ch){sl+=wl[ch];sr+=wr[ch];}
    // Finite non-negative coefficients; avoid an external libm dependency.
    const float peak=sl>sr?sl:sr;
    const float norm=peak>1.0f?peak:1.0f;
    for(int i=0;i<f->nb_samples;++i){
        float a=0,b=0;
        for(int ch=0;ch<channels;++ch){
            float v=sample_at(f->extended_data[planar?ch:0],packed,planar?(size_t)i:(size_t)i*channels+ch);
            if(!isfinite(v))v=0;else if(v>1)v=1;else if(v< -1)v=-1;
            a+=v*wl[ch];b+=v*wr[ch];
        }
        dst[2*i]=a/norm;dst[2*i+1]=b/norm;
    }
    return 0;
}
