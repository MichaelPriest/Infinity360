#pragma once
#include "HomeCanvas.h"
#include "MediaUrl.h"
#include "XsdkPaths.h"
#include "container_decode.h"
#include "av_stream_queue.h"
#include "read_clock.h"
#include "playback_controls.h"
#include "xsdk/audio.h"
#include "xsdk/xam.h"
#include "xsdk/log.h"
#include <chrono>
#include <algorithm>
#include <fstream>
#include <functional>
#if defined(INFINITY_CONTAINER_NETWORK_PROBE) || defined(INFINITY_XSDK_HTTPS)
#include "XsdkRemoteReader.h"
#include "XsdkRangeReader.h"
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
        return path.find('#')==std::string::npos&&(Suffix(target,".ts")||Suffix(target,".m3u8")||Suffix(target,".mp4"));
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
inline unsigned ControlKeys(unsigned buttons){
    return ((buttons&INPUT_GAMEPAD_A)?CloudPlaybackControls::Pause:0u)|
           ((buttons&INPUT_GAMEPAD_X)?CloudPlaybackControls::Mute:0u)|
           ((buttons&INPUT_GAMEPAD_LEFT_SHOULDER)?CloudPlaybackControls::Quieter:0u)|
           ((buttons&INPUT_GAMEPAD_RIGHT_SHOULDER)?CloudPlaybackControls::Louder:0u)|
           ((buttons&INPUT_GAMEPAD_DPAD_LEFT)?CloudPlaybackControls::Backward:0u)|
           ((buttons&INPUT_GAMEPAD_DPAD_RIGHT)?CloudPlaybackControls::Forward:0u);
}
inline std::string Play(const std::string& path,const std::string& root,HomeCanvas& canvas,
                        const std::function<bool(bool)>& present,bool diagnostic=false){
    if(!Supports(path))return "Formato FFmpeg experimental nao suportado.";
    if(!diagnostic&&!Enabled(root))return "Habilite enable-iptv-playback.txt.";
    CloudAvStreamQueue timeline;
    CloudContainerSession* session=nullptr;
    MediaUrl url;bool remoteSource=url.Parse(path);
    unsigned selectedBitrate=0;
#ifdef INFINITY_AV_NETWORK
    XsdkMpeg::RemoteReader remote;
    XsdkMpeg::RangeReader ranged;
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
            unsigned cap=CloudHls::DefaultBitrate;
            std::ifstream quality((root+"infinity360-iptv-bitrate.txt").c_str(),std::ios::binary);
            if(quality){char raw[32]{};quality.getline(raw,sizeof raw);std::string value=raw;
                if(!value.empty()&&value.back()=='\r')value.pop_back();
                if(quality.fail()||quality.peek()!=std::char_traits<char>::eof()||!CloudHls::BitrateCap(value,cap))return "Limite HLS invalido (100000 a 4000000 bps).";}
            hls.EnableBufferedSegments(true);
            if(!hls.Start(path,cap,true))return "Playlist HLS recusada ou indisponivel.";
            selectedBitrate=hls.SelectedBitrate();
            session=cloud_container_open_ts([](void* u,uint8_t* b,unsigned n){return static_cast<CloudHls::VodReader<XsdkMpeg::RemoteReader>*>(u)->Read(b,n);},&hls,CloudAvStreamQueue::Capture,&timeline);
        }else{
            const bool mp4=CloudHls::Extension(path,".mp4");
            if(mp4&&ranged.Open(path,root)){
                session=cloud_container_open_mp4_seekable(XsdkMpeg::RangeReader::ReadCallback,XsdkMpeg::RangeReader::SeekCallback,ranged.Length(),&ranged,CloudAvStreamQueue::Capture,&timeline);
            }else{
            if(Cancelled())return "Reproducao cancelada.";
            if(!remote.Open(path,root,!mp4,false,mp4))return "Fonte de video remoto nao autorizada.";
            auto read=[](void* u,uint8_t* b,unsigned n){return static_cast<XsdkMpeg::RemoteReader*>(u)->Read(b,n);};
            session=mp4?cloud_container_open_mp4(read,&remote,CloudAvStreamQueue::Capture,&timeline):cloud_container_open_ts(read,&remote,CloudAvStreamQueue::Capture,&timeline);
            }
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
    int rc=1;
    auto prepare=[&](){unsigned prefill=0;while(rc>0&&timeline.WantsInput(0)&&prefill++<4096&&!Cancelled())rc=cloud_container_step(session,1);if(rc==0)timeline.EndInput();return !Cancelled()&&rc>=0&&timeline.Prepare();};
    if(!prepare()){cloud_container_close(session);return Cancelled()?"Reproducao cancelada.":"Falha ao preparar audio e video FFmpeg.";}
    const int64_t duration=cloud_container_duration(session),mediaStart=cloud_container_start(session);
    bool audioOpen=NT_SUCCESS(xsdk_audio_open(16)),ok=audioOpen,cancelled=false;
    unsigned shown=0,submitted=0;
    using Clock=std::chrono::steady_clock;
    auto start=Clock::now();
    CloudReadClock playbackClock;
    auto wall=[&](){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-start).count();};
    INPUT_STATE initial{};
    CloudPlaybackControls controls(!remoteSource||duration>0,XamInputGetState(0,0,&initial)==ERROR_SUCCESS?ControlKeys(initial.gamepad.buttons):0);
    auto position=[&](){return std::max<int64_t>(0,timeline.Origin()-mediaStart+playbackClock.Elapsed(controls.Clock(wall())));};
    auto timeLabel=[](int64_t us){const auto seconds=us/1000000;return std::to_string(seconds/60)+":"+(seconds%60<10?"0":"")+std::to_string(seconds%60);};
    auto overlay=[&](){
        canvas.Box(30,620,1220,72,0xff172c28u,6);
        const std::string state=controls.Paused()?"Pausado":remoteSource?"Rede":"Arquivo local";
        canvas.Text(46,630,state+" | Volume "+std::to_string(controls.Volume())+"%"+(controls.Muted()?" (Mudo)":"")+(selectedBitrate?" | HLS "+std::to_string(selectedBitrate/1000)+" kbps":"")+(duration?" | "+timeLabel(std::min(duration,position()))+" / "+timeLabel(duration):""),22,0xfff2f7f3u,1160);
        canvas.Text(46,657,remoteSource&&!duration?"X Mudo | LB/RB Volume | B Sair":duration?"A Pausa | X Mudo | LB/RB Volume | Esq/Dir 30s | B Sair":"A Pausar/Retomar | X Mudo | LB/RB Volume | B Sair",19,0xffb7c9c1u,1160);
    };
    auto video=[&](const CloudAvStreamQueue::Video& v){
        canvas.VideoRgb(v.rgb.data(),v.width,v.height,v.aspect);
        ++shown;overlay();return present(true);
    };
    auto audio=[&](const float* pcm,unsigned count){
        float* out=nullptr;
        if(!NT_SUCCESS(xsdk_audio_next_frame(250,&out))||!out)return false;
        std::fill(out,out+XSDK_AUDIO_FRAME_FLOATS,0.0f);
        const float gain=controls.Gain();
        for(unsigned j=0;j<count;++j){out[j]=pcm[2*j]*gain;out[256+j]=pcm[2*j+1]*gain;}
        xsdk_audio_queue_frame();submitted+=count;return true;
    };
    while(ok){
        INPUT_STATE state{};
        const bool inputOk=XamInputGetState(0,0,&state)==ERROR_SUCCESS;
        if(inputOk&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK))){cancelled=true;break;}
        const bool changed=inputOk&&controls.Update(ControlKeys(state.gamepad.buttons),wall());
        if(changed){overlay();if(!present(true)){ok=false;break;}}
        const int seek=controls.TakeSeek();
        if(seek&&duration&&!controls.Paused()){
            const auto target=std::max<int64_t>(0,std::min<int64_t>(duration-1,position()+seek*30000000LL));
            // A bounded local keyframe seek, not frame-accurate scrubbing.
            if(audioOpen){xsdk_audio_close();audioOpen=false;}
            if(cloud_container_seek(session,target)<0){ok=false;break;}
            timeline.Reset();rc=1;
            canvas.Box(0,0,1280,720,0xff000000u);canvas.Text(45,80,"Cloud360 | Buscando quadro-chave",28,0xfff2f7f3u,1180);
            if(!present(true)||!prepare()){ok=false;break;}
            audioOpen=NT_SUCCESS(xsdk_audio_open(16));if(!audioOpen){ok=false;break;}
            controls.ResetClock();playbackClock=CloudReadClock{};start=Clock::now();
            continue;
        }
        if(controls.Paused()){if(!present(false)){ok=false;break;}continue;}
        auto elapsed=playbackClock.Elapsed(controls.Clock(wall()));
        if(timeline.Finished(elapsed))break;
        if(elapsed>CloudAvStreamQueue::MaxDuration+1000000){ok=false;break;}
        unsigned budget=4;
        while(rc>0&&timeline.WantsInput(elapsed,playbackClock.Ahead())&&budget--){
            const auto beforeRead=controls.Clock(wall());
            rc=cloud_container_step(session,1);
            const auto afterRead=controls.Clock(wall());
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
