#pragma once
#include "MpegPlayback.h"
#include "HomeCanvas.h"
#include "PlaybackHud.h"
#include "MediaUrl.h"
#include "XsdkPaths.h"
#include "xsdk/audio.h"
#include "xsdk/xam.h"
#include <fstream>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <functional>
#ifdef INFINITY_XSDK_HTTPS
#include "XsdkRemoteReader.h"
#endif
namespace XsdkMpeg {
inline bool Cancelled(){
    INPUT_STATE input{};return XamInputGetState(0,0,&input)==ERROR_SUCCESS&&(input.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK));
}
class LocalReader:public MediaReader{
    std::ifstream file_;
public:
    explicit LocalReader(const std::string& path):file_(path.c_str(),std::ios::binary){}
    bool Ready() const{return bool(file_);}
    int Read(unsigned char* data,unsigned capacity) override{
        if(Cancelled())return -1;
        file_.read(reinterpret_cast<char*>(data),capacity);
        if(file_.bad())return -1;
        return static_cast<int>(file_.gcount());
    }
};
class Output:public MpegOutput{
    HomeCanvas& canvas_;
    std::vector<unsigned char> rgb_;
    float pcm_[2][256]{};
    unsigned count_=0,phase_=0,rate_=0;
    bool audio_=false,muted_=false;
    double aspect_=1.0;
public:
    bool dirty=false;
    explicit Output(HomeCanvas& c):canvas_(c){}
    ~Output(){if(audio_)xsdk_audio_close();}
    void SetAspect(double aspect){aspect_=aspect;}
    void SetMuted(bool muted){muted_=muted;}
    bool Start(int rate){
        if(!rate)return true;
        if(rate<8000||rate>48000)return false;
        rate_=static_cast<unsigned>(rate);audio_=NT_SUCCESS(xsdk_audio_open(16));return audio_;
    }
    bool Video(plm_frame_t& frame) override{
        if(Cancelled()||!frame.width||!frame.height||frame.width>1280||frame.height>720)return false;
        rgb_.resize(frame.width*frame.height*3);
        plm_frame_to_rgb(&frame,rgb_.data(),frame.width*3);
        canvas_.VideoRgb(rgb_.data(),frame.width,frame.height,aspect_);dirty=true;return true;
    }
    bool Audio(const plm_samples_t& samples) override{
        if(!audio_||!rate_)return false;
        for(unsigned i=0;i<samples.count;++i){
            phase_+=48000;
            while(phase_>=rate_){
                phase_-=rate_;
                for(unsigned ch=0;ch<2;++ch)pcm_[ch][count_]=muted_?0.0f:std::max(-1.0f,std::min(1.0f,samples.interleaved[i*2+ch]));
                if(++count_==256){
                    if(Cancelled())return false;
                    float* frame=nullptr;if(!NT_SUCCESS(xsdk_audio_next_frame(250,&frame))||!frame)return false;
                    std::fill(frame,frame+XSDK_AUDIO_FRAME_FLOATS,0.0f);
                    std::copy(pcm_[0],pcm_[0]+256,frame);std::copy(pcm_[1],pcm_[1]+256,frame+256);
                    xsdk_audio_queue_frame();count_=0;
                }
            }
        }
        return true;
    }
};
inline std::string PlayReader(MediaReader& reader,HomeCanvas& canvas,const std::function<bool(bool)>& present,bool local=false,const PlaybackGuideOverlay::DrawCallback& drawGuide={}){
    Output output(canvas);MpegPlayback player(reader,output);
    if(!player.Open())return player.Error();
    output.SetAspect(player.PixelAspect());
    if(!output.Start(player.SampleRate()))return "Audio XSDK indisponivel.";
    using Clock=std::chrono::steady_clock;
    auto last=Clock::now();double elapsed=0;
    bool paused=false,muted=false;
    PlaybackHud hud;PlaybackGuideOverlay guide(drawGuide);const auto started=Clock::now();
    auto wall=[&](){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-started).count();};hud.Reveal(0);
    bool lastHud=true;
    auto overlay=[&](){guide.Restore(canvas);lastHud=hud.Visible(wall(),paused);hud.Draw(canvas,wall(),paused,"Cloud360 IPTV | MPEG",std::string(paused?"Pausado":"Reproduzindo")+(muted?" | Mudo":""),local?"A Pausa | X Mudo | B Sair":"X Mudo | B Sair");guide.Draw(canvas);};
    INPUT_STATE initial{};unsigned previous=XamInputGetState(0,0,&initial)==ERROR_SUCCESS?initial.gamepad.buttons:0;
    while(!Cancelled()){
        INPUT_STATE input{};bool refresh=false;
        if(XamInputGetState(0,0,&input)==ERROR_SUCCESS){
            const unsigned buttons=input.gamepad.buttons,edge=buttons&~previous;previous=buttons;
            const bool wasGuide=guide.Active();
            if((edge&INPUT_GAMEPAD_LEFT_THUMB)&&guide.Available()){guide.Toggle();refresh=true;}
            if(guide.Active()){
                if(edge&INPUT_GAMEPAD_DPAD_UP){guide.Move(-1);refresh=true;}
                if(edge&INPUT_GAMEPAD_DPAD_DOWN){guide.Move(1);refresh=true;}
            }else if(!wasGuide){
                if(edge&INPUT_GAMEPAD_START){hud.Toggle(wall());refresh=true;}
                if(edge&INPUT_GAMEPAD_X){muted=!muted;output.SetMuted(muted);hud.Reveal(wall());refresh=true;}
                if(local&&(edge&INPUT_GAMEPAD_A)){paused=!paused;last=Clock::now();hud.Reveal(wall());refresh=true;}
            }
        }
        if(lastHud!=hud.Visible(wall(),paused))refresh=true;
        if(refresh)overlay();
        if(paused){
            if(!present(refresh))return "Falha GPU.";
            last=Clock::now();continue;
        }
        auto now=Clock::now();const double delta=std::chrono::duration<double>(now-last).count();
        if(delta<0.025){if(!present(refresh))return "Falha GPU.";continue;}
        last=now;elapsed=std::min(0.1,delta);
        const bool running=player.Advance(elapsed);
        if(output.dirty){guide.NewFrame();hud.Frame(canvas);overlay();}
        if(!present(output.dirty||refresh))return "Falha GPU.";
        output.dirty=false;
        if(!running)return player.Error().empty()?"Reproducao encerrada.":player.Error();
    }
    return "Reproducao cancelada.";
}
inline std::string Play(const std::string& path,const std::string& root,HomeCanvas& canvas,const std::function<bool(bool)>& present,const PlaybackGuideOverlay::DrawCallback& drawGuide={}){
    std::ifstream marker((root+"enable-iptv-playback.txt").c_str());std::string enabled;
    if(!std::getline(marker,enabled)||enabled!="ENABLE-IPTV-PLAYBACK")return "Habilite enable-iptv-playback.txt.";
    MediaUrl url;
    if(url.Parse(path)){
#ifdef INFINITY_XSDK_HTTPS
        RemoteReader reader;if(!reader.Open(path,root))return "Fonte MPEG-PS nao autorizada ou indisponivel.";
        return PlayReader(reader,canvas,present,false,drawGuide);
#else
        return "Transporte de rede indisponivel.";
#endif
    }
    const auto local=XsdkPaths::Normalize(path);
    if(local.empty()||(!tvEndsWith(local,".mpg")&&!tvEndsWith(local,".mpeg")&&!tvEndsWith(local,".ps")))return "Use MPEG-1/MP2 em MPEG-PS; WMV nao suportado neste XSDK.";
    LocalReader reader(local);if(!reader.Ready())return "Arquivo local indisponivel.";
    return PlayReader(reader,canvas,present,true,drawGuide);
}
}
