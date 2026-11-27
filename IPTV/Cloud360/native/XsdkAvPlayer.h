#pragma once
#include "HomeCanvas.h"
#include "PlaybackHud.h"
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
inline std::string PlayAttempt(const std::string& path,const std::string& root,HomeCanvas& canvas,
                        const std::function<bool(bool)>& present,bool diagnostic,const PlaybackGuideOverlay::DrawCallback& drawGuide,bool& retryLive,unsigned& adaptiveCap,CloudPlaybackControls::AudioPreference& audioPreference,bool allowSwitch,bool allowUpgrade,bool& upgrade,unsigned& previousBitrate,bool& played,unsigned& manualCap,bool allowManual,bool& manualChange){
    retryLive=false;upgrade=false;previousBitrate=0;played=false;manualChange=false;
    if(!Supports(path))return "Formato FFmpeg experimental nao suportado.";
    if(!diagnostic&&!Enabled(root))return "Habilite enable-iptv-playback.txt.";
    CloudAvStreamQueue timeline;
    CloudContainerSession* session=nullptr;
    MediaUrl url;bool remoteSource=url.Parse(path);
    unsigned selectedBitrate=0;
#ifdef INFINITY_AV_NETWORK
    XsdkMpeg::RemoteReader remote;
    XsdkMpeg::RangeReader ranged;
    std::string hlsError;unsigned userHlsCap=CloudHls::DefaultBitrate;
    CloudHls::VodReader<XsdkMpeg::RemoteReader> hls([&](const std::string& url,bool playlist){
        auto r=std::make_unique<XsdkMpeg::RemoteReader>();
        if(!r->Open(url,root,!playlist,playlist,!playlist&&CloudHls::Extension(url,".mp4"),!playlist&&CloudHls::Extension(url,".m4s"))){hlsError=r->Error();return std::unique_ptr<XsdkMpeg::RemoteReader>{};}
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
            userHlsCap=cap;
            if(adaptiveCap)cap=std::min(cap,adaptiveCap);
            if(manualCap)cap=std::min(cap,manualCap);
            // Keep automatic HLS selection at 720p unless explicitly enabled.
            std::ifstream fullHd((root+"enable-iptv-1080p.txt").c_str(),std::ios::binary);
            if(fullHd){std::string marker;std::getline(fullHd,marker);if(!marker.empty()&&marker.back()=='\r')marker.pop_back();
                if(marker=="ENABLE-IPTV-1080P"&&fullHd.peek()==std::char_traits<char>::eof())hls.EnableFullHd();}
            hls.EnableBufferedSegments(true);
            hls.EnablePrefetch([](XsdkMpeg::RemoteReader& r){return r.Prefetch();});
            if(!manualCap)hls.EnableStartupAdaptation([](){return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());});
            if(!hls.Start(path,cap,true))return hlsError.empty()?"Playlist HLS recusada ou indisponivel.":hlsError;
            selectedBitrate=hls.SelectedBitrate();
            auto readHls=[](void* u,uint8_t* b,unsigned n){return static_cast<CloudHls::VodReader<XsdkMpeg::RemoteReader>*>(u)->Read(b,n);};
            session=hls.Fragmented()?cloud_container_open_mp4(readHls,&hls,CloudAvStreamQueue::Capture,&timeline):cloud_container_open_ts(readHls,&hls,CloudAvStreamQueue::Capture,&timeline);
        }else{
            const bool mp4=CloudHls::Extension(path,".mp4");
            if(mp4&&ranged.Open(path,root)){
                session=cloud_container_open_mp4_seekable(XsdkMpeg::RangeReader::ReadCallback,XsdkMpeg::RangeReader::SeekCallback,ranged.Length(),&ranged,CloudAvStreamQueue::Capture,&timeline);
            }else{
            if(Cancelled())return "Reproducao cancelada.";
            if(!remote.Open(path,root,!mp4,false,mp4))return remote.Error();
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
    if(!session){
#ifdef INFINITY_AV_NETWORK
        if(hls.LastFailure()!=CloudHls::Failure::None)return CloudHls::FailureLabel(hls.LastFailure());
#endif
        return Cancelled()?"Reproducao cancelada.":"Fonte FFmpeg indisponivel ou incompativel.";
    }
    int rc=1;
    auto prepare=[&](){unsigned prefill=0;while(rc>0&&timeline.WantsInput(0)&&prefill++<4096&&!Cancelled())rc=cloud_container_step(session,1);if(rc==0)timeline.EndInput();return !Cancelled()&&rc>=0&&timeline.Prepare();};
    if(!prepare()){cloud_container_close(session);return Cancelled()?"Reproducao cancelada.":"Falha ao preparar audio e video FFmpeg.";}
    const int64_t duration=cloud_container_duration(session),mediaStart=cloud_container_start(session);
    bool audioOpen=NT_SUCCESS(xsdk_audio_open(16)),ok=audioOpen,cancelled=false;
    unsigned shown=0,submitted=0,sourceWidth=0,sourceHeight=0;
    using Clock=std::chrono::steady_clock;
    auto start=Clock::now();
    CloudReadClock playbackClock;
    auto wall=[&](){return std::chrono::duration_cast<std::chrono::microseconds>(Clock::now()-start).count();};
    INPUT_STATE initial{};
    CloudPlaybackControls controls(!remoteSource||duration>0,XamInputGetState(0,0,&initial)==ERROR_SUCCESS?ControlKeys(initial.gamepad.buttons):0);
    controls.RestoreAudio(audioPreference);
#ifdef INFINITY_AV_NETWORK
    if(allowSwitch&&!manualCap)hls.ArmRuntimeAdaptation(allowUpgrade?userHlsCap:selectedBitrate);
    const bool qualityAvailable=allowManual&&!hls.QualityCaps(userHlsCap).empty();
#endif
    auto position=[&](){return std::max<int64_t>(0,timeline.Origin()-mediaStart+playbackClock.Elapsed(controls.Clock(wall())));};
    auto timeLabel=[](int64_t us){const auto seconds=us/1000000;return std::to_string(seconds/60)+":"+(seconds%60<10?"0":"")+std::to_string(seconds%60);};
    std::string qualityHint;
#ifdef INFINITY_AV_NETWORK
    if(qualityAvailable)qualityHint=" | Y Qualidade";
#endif
    PlaybackHud hud;hud.Reveal(wall());PlaybackGuideOverlay guide(drawGuide);
    unsigned held=XamInputGetState(0,0,&initial)==ERROR_SUCCESS?initial.gamepad.buttons:0;
    bool lastHud=true;
    bool liveAudioFailure=false,showDetails=false;
    CloudQualityMenu qualityMenu;
    CloudContainerInfo mediaInfo{};cloud_container_info(session,&mediaInfo);
    auto overlay=[&](){
        guide.Restore(canvas);
        const std::string state=controls.Paused()?"Pausado":remoteSource?"Rede":"Arquivo local";
        const auto info=state+" | Volume "+std::to_string(controls.Volume())+"%"+(controls.Muted()?" (Mudo)":"")+(selectedBitrate?" | HLS "+std::to_string(selectedBitrate/1000)+" kbps":"")+(sourceWidth>1280||sourceHeight>720?" | "+std::to_string(sourceWidth)+"x"+std::to_string(sourceHeight)+" > 720p":"")+(duration>0?" | "+timeLabel(std::min(duration,position()))+" / "+timeLabel(duration):"");
        std::string bufferInfo=remoteSource?(" | Fila "+std::to_string(timeline.Bytes()/1024)+" KiB | AV "+std::to_string(timeline.BufferedAhead(playbackClock.Elapsed(controls.Clock(wall())))/1000)+" ms | Alvo "+std::to_string(playbackClock.Ahead()/1000)+" ms"):"";
#ifdef INFINITY_AV_NETWORK
        if(remoteSource&&hls.RecentThroughput())bufferInfo+=" | Rede "+std::to_string(hls.RecentThroughput()/1000)+" kbps";
        if(selectedBitrate)bufferInfo+=manualCap?(" | Manual ate "+std::to_string(manualCap/1000)+" kbps"):" | Auto";
#endif
        std::string detailLine;
        if(showDetails){
            detailLine=std::string(mediaInfo.video_codec?mediaInfo.video_codec:"Video ?")+" / "+(mediaInfo.audio_codec?mediaInfo.audio_codec:"Audio ?");
            if(sourceWidth&&sourceHeight)detailLine+=" | Fonte "+std::to_string(sourceWidth)+"x"+std::to_string(sourceHeight);
            if(mediaInfo.fps_den)detailLine+=" | FPS nominal "+std::to_string((uint64_t(mediaInfo.fps_num)*10/mediaInfo.fps_den)/10)+"."+std::to_string((uint64_t(mediaInfo.fps_num)*10/mediaInfo.fps_den)%10);
            if(mediaInfo.sample_rate)detailLine+=" | Audio "+std::to_string(mediaInfo.sample_rate)+" Hz";
            detailLine+=" | Quadros descartados "+std::to_string(timeline.Dropped())+" | Pausas leitura "+std::to_string(playbackClock.Stalls());
        }
        if(qualityMenu.Active()){
            const unsigned choice=qualityMenu.Choice();
            detailLine="Qualidade: < "+(choice?std::to_string(choice/1000)+" kbps (teto)":std::string("Automatica"))+" >  | "+std::to_string(qualityMenu.Index()+1)+"/"+std::to_string(qualityMenu.Count())+(choice==manualCap?" | Atual":" | A Aplicar");
        }
        lastHud=hud.Visible(wall(),controls.Paused()||showDetails||qualityMenu.Active());
        hud.Draw(canvas,wall(),controls.Paused()||showDetails||qualityMenu.Active(),"Cloud360 IPTV | FFmpeg",info+bufferInfo,qualityMenu.Active()?"Esq/Dir Selecionar | A Aplicar | Y Voltar | B Sair":remoteSource&&duration<=0?("X Mudo | LB/RB Volume | B Sair | Baixo Info"+qualityHint):duration>0?"A Pausa | X Mudo | LB/RB Volume | Esq/Dir 30s | B Sair":"A Pausar/Retomar | X Mudo | LB/RB Volume | B Sair",duration>0?int(100.0*std::min(duration,position())/duration):-1,detailLine);
        guide.Draw(canvas);
    };
    auto video=[&](const CloudAvStreamQueue::Video& v){
        const auto* report=cloud_container_report(session);if(report){sourceWidth=report->width;sourceHeight=report->height;}
        canvas.VideoRgb(v.rgb.data(),v.width,v.height,v.aspect);
        guide.NewFrame();hud.Frame(canvas);++shown;overlay();return present(true);
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
        const unsigned edges=inputOk?state.gamepad.buttons&~held:0;
        if(inputOk)held=state.gamepad.buttons;
        const bool guideEdge=(edges&INPUT_GAMEPAD_LEFT_THUMB)&&guide.Available();
        const bool wasGuide=guide.Active();if(guideEdge)guide.Toggle();
        bool guideChanged=guideEdge;
        if(guideEdge&&qualityMenu.Active())qualityMenu.Close();
        if(guide.Active()){
            if(edges&INPUT_GAMEPAD_DPAD_UP){guide.Move(-1);guideChanged=true;}
            if(edges&INPUT_GAMEPAD_DPAD_DOWN){guide.Move(1);guideChanged=true;}
        }
        bool changed=false;
#ifdef INFINITY_AV_NETWORK
        const bool wasQuality=qualityMenu.Active();
        if(inputOk&&!guide.Active()&&!wasGuide&&!guideEdge){
            if(wasQuality){
                if(edges&INPUT_GAMEPAD_Y){qualityMenu.Close();changed=true;}
                else if(edges&INPUT_GAMEPAD_A){
                    if(qualityMenu.Confirm(manualCap)){
                        previousBitrate=selectedBitrate;adaptiveCap=0;manualChange=true;retryLive=true;break;
                    }
                    changed=true;
                }else if(edges&(INPUT_GAMEPAD_DPAD_LEFT|INPUT_GAMEPAD_DPAD_RIGHT)){
                    qualityMenu.Move((edges&INPUT_GAMEPAD_DPAD_LEFT)?-1:1);changed=true;
                }
            }else if(qualityAvailable&&(edges&INPUT_GAMEPAD_Y)&&shown&&submitted){
                changed=qualityMenu.Open(hls.QualityCaps(userHlsCap),manualCap);showDetails=false;
            }
        }
#else
        const bool wasQuality=false;
#endif
        if(inputOk&&(guide.Active()||wasGuide||guideEdge||wasQuality||qualityMenu.Active()))controls.SynchronizeKeys(ControlKeys(state.gamepad.buttons));
        else if(inputOk){
            changed=controls.Update(ControlKeys(state.gamepad.buttons),wall());
            if(remoteSource&&duration<=0&&(edges&INPUT_GAMEPAD_DPAD_DOWN)){showDetails=!showDetails;hud.Reveal(wall());changed=true;}
            if(edges&INPUT_GAMEPAD_START){showDetails=false;hud.Toggle(wall());changed=true;}
            else if(edges&&ControlKeys(edges))hud.Reveal(wall());
        }
        if(changed||guideChanged||lastHud!=hud.Visible(wall(),controls.Paused()||showDetails||qualityMenu.Active())){overlay();if(!present(true)){ok=false;break;}}
        const int seek=controls.TakeSeek();
        if(seek&&duration>0&&!controls.Paused()){
            const auto target=std::max<int64_t>(0,std::min<int64_t>(duration-1,position()+seek*30000000LL));
            // A bounded local keyframe seek, not frame-accurate scrubbing.
            if(audioOpen){xsdk_audio_close();audioOpen=false;}
            if(cloud_container_seek(session,target)<0){ok=false;break;}
            timeline.Reset();rc=1;guide.NewFrame();hud.Clear();hud.Reveal(0);
            canvas.Box(0,0,1280,720,0xff000000u);canvas.Text(45,80,"Cloud360 | Buscando quadro-chave",28,0xfff2f7f3u,1180);
            if(!present(true)||!prepare()){ok=false;break;}
            audioOpen=NT_SUCCESS(xsdk_audio_open(16));if(!audioOpen){ok=false;break;}
            controls.ResetClock();playbackClock=CloudReadClock{};start=Clock::now();
            continue;
        }
        if(controls.Paused()){if(!present(false)){ok=false;break;}continue;}
#ifdef INFINITY_AV_NETWORK
        // Advance existing network transfers cooperatively while A/V frames are queued.
        // The callback never calls curl_multi_poll or waits for media data.
        if(remoteSource){hls.Prefetch();if(!hls.Live()&&!selectedBitrate)remote.Prefetch();}
#endif
        auto elapsed=playbackClock.Elapsed(controls.Clock(wall()));
        if(timeline.Finished(elapsed))break;
        if(elapsed>CloudAvStreamQueue::MaxDuration+1000000){ok=false;break;}
        // Present already-due frames before any potentially blocking network read.
#ifdef INFINITY_AV_NETWORK
        if(hls.Live()&&!timeline.Pump(elapsed,video,audio)){ok=false;break;}
#endif
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
    played=shown&&submitted;
    const auto pumpFailure=timeline.LastPumpFailure();
    liveAudioFailure=pumpFailure==CloudAvStreamQueue::PumpFailure::AudioLate||pumpFailure==CloudAvStreamQueue::PumpFailure::AudioSink;
    xsdk_log_write("Cloud360 AV pump failure=%u",static_cast<unsigned>(pumpFailure));
    audioPreference=controls.Audio();
    cloud_container_close(session);
    if(audioOpen)xsdk_audio_close();
    xsdk_log_write("Cloud360 AV timeline ok=%d cancelled=%d frames=%u samples=%u dropped=%u",ok,cancelled,shown,submitted,static_cast<unsigned>(timeline.Dropped()));
    xsdk_log_write("Cloud360 read stalls=%u compensated_ms=%u ahead_ms=%u",playbackClock.Stalls(),static_cast<unsigned>(playbackClock.Stalled()/1000),static_cast<unsigned>(playbackClock.Ahead()/1000));
    if(cancelled||Cancelled())return "Reproducao cancelada.";
    if(manualChange)return manualCap?("Qualidade manual ate "+std::to_string(manualCap/1000)+" kbps"):"Qualidade automatica";
#ifdef INFINITY_AV_NETWORK
    xsdk_log_write("Cloud360 prefetched HLS segments=%u remote_peak=%u",hls.PrefetchedSegments(),static_cast<unsigned>(remote.Peak()));
    if(hls.LastFailure()!=CloudHls::Failure::None){
        // Restart only a previously playing live HLS stream after transport/window failure.
        // All decoder/reader/audio objects are destroyed before restarting; never concatenate streams.
        const bool quality=(hls.LastFailure()==CloudHls::Failure::LowerQuality||hls.LastFailure()==CloudHls::Failure::HigherQuality)&&hls.SuggestedBitrate()>0;
        retryLive=!ok&&played&&hls.Live()&&(CloudHls::RecoverLive(hls.LastFailure())||quality);
        if(retryLive&&quality){upgrade=hls.LastFailure()==CloudHls::Failure::HigherQuality;previousBitrate=selectedBitrate;adaptiveCap=hls.SuggestedBitrate();}
        if(retryLive&&!quality&&selectedBitrate)adaptiveCap=adaptiveCap?std::min(adaptiveCap,selectedBitrate):selectedBitrate;
        return CloudHls::FailureLabel(hls.LastFailure());
    }
    // Recover only a previously playing live HLS source, through a clean
    // decoder/audio session. The outer loop enforces the existing retry budget.
    if(!ok&&played&&hls.Live()&&liveAudioFailure){
        retryLive=true;
        if(selectedBitrate)adaptiveCap=adaptiveCap?std::min(adaptiveCap,selectedBitrate):selectedBitrate;
        return "Audio atrasado ou indisponivel; resincronizando transmissao.";
    }
#endif
    return ok&&shown&&submitted?"Reproducao encerrada.":"Falha na reproducao FFmpeg.";
}
inline std::string Play(const std::string& path,const std::string& root,HomeCanvas& canvas,
                        const std::function<bool(bool)>& present,bool diagnostic=false,const PlaybackGuideOverlay::DrawCallback& drawGuide={}){
    std::string result;unsigned adaptiveCap=0;CloudPlaybackControls::AudioPreference audioPreference;
    unsigned normalRetries=0,rollbackCap=0,manualCap=0,rollbackManual=0,manualChanges=0;bool upgradeUsed=false;
    // Two ordinary retries, one upgrade/rollback, and four user changes with rollback.
    for(unsigned attempt=0;attempt<13;++attempt){
        bool retry=false,upgrade=false,played=false,manualChange=false;unsigned previousBitrate=0;
        const unsigned oldManual=manualCap;
        result=PlayAttempt(path,root,canvas,present,diagnostic,drawGuide,retry,adaptiveCap,audioPreference,
            normalRetries<2,!upgradeUsed,upgrade,previousBitrate,played,manualCap,manualChanges<4,manualChange);
        if(Cancelled()||result=="Reproducao cancelada.")return "Reproducao cancelada.";
        if(rollbackCap&&!played&&!retry){
            adaptiveCap=rollbackCap;manualCap=rollbackManual;rollbackCap=0;retry=true;
            result="Qualidade superior indisponivel; retornando a anterior.";
        }else{
            rollbackCap=0;
            if(!retry)return result;
            if(manualChange&&manualChanges<4){++manualChanges;rollbackCap=previousBitrate;rollbackManual=oldManual;}
            else if(upgrade&&!upgradeUsed){upgradeUsed=true;rollbackCap=previousBitrate;rollbackManual=oldManual;}
            else if(normalRetries<2){++normalRetries;}
            else return result;
        }
        if(attempt==12)return result;
        canvas.Background();canvas.SeriesTile(160,240,960,220,false,220);
        canvas.TextFit(190,265,"Cloud360 | Reconectando transmissao ao vivo",30,0xfff2f7f3u,900);
        canvas.TextFit(190,320,result,22,0xffb7c9c1u,900);
        canvas.TextFit(190,379,"Sessao "+std::to_string(attempt+2)+(adaptiveCap?" | Ate "+std::to_string(adaptiveCap/1000)+" kbps":"")+" | B Cancelar",20,0xff9dec31u,900);
        if(!present(true))return "Falha ao apresentar reconexao IPTV.";
        const auto start=std::chrono::steady_clock::now();
        while(std::chrono::steady_clock::now()-start<std::chrono::milliseconds(manualChange?250:500*std::min(attempt+1,3u))){
            if(Cancelled())return "Reproducao cancelada.";
            if(!present(false))return "Falha ao apresentar reconexao IPTV.";
        }
    }
    return result;
}
}
