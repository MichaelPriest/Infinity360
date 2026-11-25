#pragma once
#include "HomeCanvas.h"
#include "MediaUrl.h"
#include "XsdkPaths.h"
#include "container_decode.h"
#include "av_stream_queue.h"
#include "read_clock.h"
#include "xsdk/audio.h"
#include "xsdk/xam.h"
#include "xsdk/log.h"
#include <chrono>
#include <algorithm>
#include <fstream>
#include <functional>
#if defined(INFINITY_CONTAINER_NETWORK_PROBE) || defined(INFINITY_XSDK_HTTPS)
#include "XsdkRemoteReader.h"
#include "hls_vod.h"
#define INFINITY_AV_NETWORK 1
#endif
namespace XsdkAv {
inline bool Cancelled(){
    INPUT_STATE state{};
    return XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK));
}
inline std::string Lower(std::string s){for(char& c:s)if(c>='A'&&c<='Z')c+=32;return s;}
inline bool Suffix(const std::string& path,const std::string& ext){
    const auto p=Lower(path);return p.size()>=ext.size()&&p.compare(p.size()-ext.size(),ext.size(),ext)==0;
}
inline bool Supports(const std::string& path){
    MediaUrl url;
    if(url.Parse(path)){
#ifdef INFINITY_AV_NETWORK
        const auto target=url.target.substr(0,url.target.find('?'));
        return path.find('#')==std::string::npos&&(Suffix(target,".ts")||Suffix(target,".m3u8"));
#else
        return false;
#endif
    }
    return Suffix(path,".mp4")||Suffix(path,".ts");
}
inline bool Enabled(const std::string& root){
    std::ifstream file((root+"enable-iptv-playback.txt").c_str());std::string marker;
    return std::getline(file,marker)&&marker=="ENABLE-IPTV-PLAYBACK";
}
inline std::string Play(const std::string& path,const std::string& root,HomeCanvas& canvas,
                        const std::function<bool(bool)>& present,bool diagnostic=false){
    if(!Supports(path))return "Formato FFmpeg experimental nao suportado.";
    if(!diagnostic&&!Enabled(root))return "Habilite enable-iptv-playback.txt.";
    CloudAvStreamQueue timeline;
    CloudContainerSession* session=nullptr;
    MediaUrl url;bool remoteSource=url.Parse(path);
#ifdef INFINITY_AV_NETWORK
    XsdkMpeg::RemoteReader remote;
    CloudHls::VodReader<XsdkMpeg::RemoteReader> hls([&](const std::string& url,bool playlist){
        auto r=std::make_unique<XsdkMpeg::RemoteReader>();
        if(!r->Open(url,root,!playlist,playlist))return std::unique_ptr<XsdkMpeg::RemoteReader>{};
        return r;
    },[](unsigned ms){
        if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)return false;
        CURLM* timer=curl_multi_init();bool ok=timer!=nullptr;
        while(ok&&ms){
            if(Cancelled()){ok=false;break;}
            unsigned slice=std::min(ms,50u);int activity=0;
            ok=curl_multi_poll(timer,nullptr,0,static_cast<int>(slice),&activity)==CURLM_OK;ms-=slice;
        }
        if(timer)curl_multi_cleanup(timer);curl_global_cleanup();return ok;
    });
    if(remoteSource){
        if(!Enabled(root))return "Habilite enable-iptv-playback.txt.";
        if(CloudHls::Extension(path,".m3u8")){
            if(!hls.Start(path,2000000,true))return "Playlist HLS recusada ou indisponivel.";
            session=cloud_container_open_ts([](void* u,uint8_t* b,unsigned n){return static_cast<CloudHls::VodReader<XsdkMpeg::RemoteReader>*>(u)->Read(b,n);},&hls,CloudAvStreamQueue::Capture,&timeline);
        }else{
            if(!remote.Open(path,root,true))return "Fonte MPEG-TS nao autorizada.";
            session=cloud_container_open_ts([](void* u,uint8_t* b,unsigned n){return static_cast<XsdkMpeg::RemoteReader*>(u)->Read(b,n);},&remote,CloudAvStreamQueue::Capture,&timeline);
        }
    }else
#endif
    {
        if(remoteSource)return "Rede FFmpeg indisponivel.";
        const auto local=XsdkPaths::Normalize(path);
        if(local.empty())return "Caminho local invalido.";
        session=cloud_container_open(local.c_str(),CloudAvStreamQueue::Capture,&timeline,1);
    }
    if(!session)return "Fonte FFmpeg indisponivel ou incompativel.";
    int rc=1;unsigned prefill=0;
    while(rc>0&&timeline.WantsInput(0)&&prefill++<4096&&!Cancelled())rc=cloud_container_step(session,1);
    if(rc==0)timeline.EndInput();
    if(Cancelled()){cloud_container_close(session);return "Reproducao cancelada.";}
    if(rc<0||!timeline.Prepare()){cloud_container_close(session);return "Falha ao preparar audio e video FFmpeg.";}
    bool audioOpen=NT_SUCCESS(xsdk_audio_open(16)),ok=audioOpen,cancelled=false;
    unsigned shown=0,submitted=0;
    using Clock=std::chrono::steady_clock;
    const auto start=Clock::now();
    CloudReadClock playbackClock;
    auto wall=[&](){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-start).count();};
    auto video=[&](const CloudAvStreamQueue::Video& v){
        canvas.VideoRgb(v.rgb.data(),v.width,v.height,v.aspect);
        ++shown;return present(true);
    };
    auto audio=[&](const float* pcm,unsigned count){
        float* out=nullptr;
        if(!NT_SUCCESS(xsdk_audio_next_frame(250,&out))||!out)return false;
        std::fill(out,out+XSDK_AUDIO_FRAME_FLOATS,0.0f);
        for(unsigned j=0;j<count;++j){out[j]=pcm[2*j];out[256+j]=pcm[2*j+1];}
        xsdk_audio_queue_frame();submitted+=count;return true;
    };
    while(ok){
        INPUT_STATE state{};
        if(XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK))){cancelled=true;break;}
        auto elapsed=playbackClock.Elapsed(wall());
        if(timeline.Finished(elapsed))break;
        if(elapsed>CloudAvStreamQueue::MaxDuration+1000000){ok=false;break;}
        unsigned budget=4;
        while(rc>0&&timeline.WantsInput(elapsed,playbackClock.Ahead())&&budget--){
            const auto beforeRead=wall();
            rc=cloud_container_step(session,1);
            const auto afterRead=wall();
            if(remoteSource&&!playbackClock.Observe(beforeRead,afterRead)){ok=false;break;}
            elapsed=playbackClock.Elapsed(afterRead);
        }
        if(rc==0)timeline.EndInput();
        if(!ok||rc<0){ok=false;break;}
        const auto before=shown;
        ok=timeline.Pump(elapsed,video,audio);
        if(ok&&shown==before)ok=present(false);
    }
    cloud_container_close(session);
    if(audioOpen)xsdk_audio_close();
    xsdk_log_write("Cloud360 AV timeline ok=%d cancelled=%d frames=%u samples=%u dropped=%u",ok,cancelled,shown,submitted,static_cast<unsigned>(timeline.Dropped()));
    xsdk_log_write("Cloud360 read stalls=%u compensated_ms=%u ahead_ms=%u",playbackClock.Stalls(),static_cast<unsigned>(playbackClock.Stalled()/1000),static_cast<unsigned>(playbackClock.Ahead()/1000));
    return cancelled||Cancelled()?"Reproducao cancelada.":ok&&shown&&submitted?"Reproducao encerrada.":"Falha na reproducao FFmpeg.";
}
}
