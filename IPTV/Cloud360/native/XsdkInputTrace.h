#pragma once
#include "HomeCanvas.h"
#include "InfinityVersion.h"
#include "xsdk/xam.h"
#include "xsdk/log.h"
#include <cstdio>
#include <fstream>
#include <string>
// Opt-in observation only: never substitutes input or bypasses the official Guide.
class XsdkInputTrace {
    bool enabled_=false;
    unsigned entries_=0,frame_=0,result_=0,raw_=0,normalized_=0,rawEdge_=0,gateEdge_=0,dispatch_=0;
    int lx_=0,ly_=0;
    bool ui_=false,idle_=false,seen_=false;
    std::ofstream log_;
    std::string last_;
    void Emit(const std::string& text){
        if(entries_>=256)return;
        ++entries_;
        xsdk_log_write("%s",text.c_str());
        if(log_){log_<<text<<"\n";log_.flush();}
    }
public:
    explicit XsdkInputTrace(const std::string& root){
        std::ifstream flag((root+"enable-input-diagnostics.txt").c_str());
        std::string line;
        enabled_=bool(std::getline(flag,line))&&(line=="ENABLE-INPUT-DIAGNOSTICS"||line=="ENABLE-INPUT-DIAGNOSTICS\r");
        if(enabled_){
            log_.open((root+"UserFiles/input-diagnostic.log").c_str(),std::ios::out|std::ios::trunc);
            Emit("INFINITY INPUT " INFINITY360_VERSION " | slot 0 | original XAM input");
        }
    }
    bool Enabled()const{return enabled_;}
    bool Observe(unsigned frame,unsigned result,const INPUT_STATE& in,unsigned normalized,unsigned rawEdge,unsigned gated,bool ui){
        if(!enabled_)return false;
        const bool changed=!seen_||result_!=result||raw_!=in.gamepad.buttons||normalized_!=normalized||ui_!=ui||rawEdge_!=rawEdge||gateEdge_!=gated||((lx_>16000)!=(in.gamepad.thumb_lx>16000))||((lx_<-16000)!=(in.gamepad.thumb_lx<-16000))||((ly_>16000)!=(in.gamepad.thumb_ly>16000))||((ly_<-16000)!=(in.gamepad.thumb_ly<-16000));
        seen_=true;frame_=frame;result_=result;raw_=in.gamepad.buttons;normalized_=normalized;rawEdge_=rawEdge;gateEdge_=gated;ui_=ui;lx_=in.gamepad.thumb_lx;ly_=in.gamepad.thumb_ly;
        if(changed||frame%120==0){
            char buf[220];
            std::snprintf(buf,sizeof buf,"INPUT frame=%u rc=%08X raw=%04X lx=%d ly=%d mapped=%04X edge=%04X gated=%04X sysui=%u",frame,result,raw_,lx_,ly_,normalized,rawEdge,gated,unsigned(ui));
            Emit(buf);
            return true;
        }
        return false;
    }
    void Dispatch(unsigned edge,bool idle){
        if(!enabled_)return;
        if(edge!=dispatch_||idle!=idle_){
            char buf[100];std::snprintf(buf,sizeof buf,"DISPATCH frame=%u edge=%04X idle=%u",frame_,edge,unsigned(idle));Emit(buf);
        }
        dispatch_=edge;idle_=idle;
    }
    void Draw(HomeCanvas& c)const{
        char buf[180];
        c.Box(35,6,1210,65,0xff071615u,6);
        std::snprintf(buf,sizeof buf,"INPUT " INFINITY360_VERSION " | Frame %u | XAM %08X | RAW %04X | LX %d LY %d",frame_,result_,raw_,lx_,ly_);
        c.TextFit(48,12,buf,18,0xffffffffu,1180);
        std::snprintf(buf,sizeof buf,"Mapped %04X | Edge %04X | Gated %04X | Dispatch %04X | SYSUI %u | Idle %u",normalized_,rawEdge_,gateEdge_,dispatch_,unsigned(ui_),unsigned(idle_));
        c.TextFit(48,38,buf,17,0xffa8e96du,1180);
    }
};
