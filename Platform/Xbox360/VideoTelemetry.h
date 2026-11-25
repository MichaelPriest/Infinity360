#pragma once
#include <cstdint>
#include <cmath>
#include <string>
#ifdef __XSDK__
#include "xsdk/kernel.h"
#endif

// Read-only video mode reported by the kernel, not a GPU benchmark or a
// guarantee about the signal negotiated with the television.
struct VideoTelemetry {
    bool available=false,displayAvailable=false,interlaced=false,widescreen=false,hiDef=false;
    unsigned width=0,height=0,refreshMilliHz=0,standard=0;
    unsigned displayWidth=0,displayHeight=0,bufferWidth=0,bufferHeight=0;
    std::string kernel;
    static bool Dimensions(unsigned w,unsigned h){return w>=160&&h>=120&&w<=4096&&h<=2160;}
    static VideoTelemetry FromMode(unsigned w,unsigned h,float hz,unsigned interlace,unsigned wide,unsigned hd,unsigned standard){
        VideoTelemetry result;
        if(!Dimensions(w,h)||!std::isfinite(hz)||hz<20.0f||hz>240.0f||interlace>1||wide>1||hd>1)return result;
        result.available=true;result.width=w;result.height=h;
        result.refreshMilliHz=static_cast<unsigned>(hz*1000.0f+0.5f);
        result.interlaced=interlace!=0;result.widescreen=wide!=0;result.hiDef=hd!=0;result.standard=standard;
        return result;
    }
    void SetDisplay(unsigned w,unsigned h,unsigned bw,unsigned bh){
        displayAvailable=Dimensions(w,h)&&Dimensions(bw,bh);
        displayWidth=displayAvailable?w:0;displayHeight=displayAvailable?h:0;
        bufferWidth=displayAvailable?bw:0;bufferHeight=displayAvailable?bh:0;
    }
    std::string Frequency()const{
        if(!available)return "-";
        const unsigned frac=refreshMilliHz%1000;
        const auto digits=std::to_string(1000+frac).substr(1);
        return std::to_string(refreshMilliHz/1000)+"."+digits+" Hz";
    }
    static VideoTelemetry Read(){
        VideoTelemetry result;
#ifdef __XSDK__
        VIDEO_MODE mode{};VdQueryVideoMode(&mode);
        result=FromMode(mode.display_width,mode.display_height,mode.refresh_rate,mode.is_interlaced,mode.is_widescreen,mode.is_hi_def,mode.video_standard);
        DISPLAY_INFO display{};VdGetCurrentDisplayInformation(&display);
        result.SetDisplay(display.display_width,display.display_height,display.front_buffer_width,display.front_buffer_height);
        if(XboxKrnlVersion)result.kernel=std::to_string(XboxKrnlVersion->major)+"."+std::to_string(XboxKrnlVersion->minor)+"."+std::to_string(XboxKrnlVersion->build)+"."+std::to_string(XboxKrnlVersion->qfe);
#endif
        return result;
    }
};
