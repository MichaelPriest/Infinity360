// Isolated, bounded GPU test of production canvas. No writes to profiles,
// no launch, no playback, no network. Test only after no-GPU diagnostics.
#include "HomeCanvas.h"
#include "DisplaySafeArea.h"
#include "GameScanner.h"
#include "TvCatalog.h"
#ifdef INFINITY_FULL_MENU_PREVIEW
#include "ConsoleHome.h"
#include "DashboardMotion.h"
#include "NativeSocialUiXsdk.h"
#include "NativeProfilePrivilegesXsdk.h"
#include "NativeAvatarProbeXsdk.h"
#include "NativeAvatarBodyXsdk.h"
#include "NativeAvatarManifestXsdk.h"
#include "NativeFriendRosterXsdk.h"
#include "NativeSocialListenerXsdk.h"
#include "XexLaunchCheck.h"
#include "DashboardScreenshot.h"
#include "XsdkPackageMount.h"
#ifdef INFINITY_XSDK_HTTPS
#include "XsdkDownload.h"
#ifdef INFINITY_STORE_ZIP
#include "StoreZipInstaller.h"
#endif
#include "DiscInstaller.h"
#include "StorageInfo.h"
#include "ArtImport.h"
#include "ContentBackup.h"
#include <chrono>
#endif
#include "xsdk/xam.h"
#include "XsdkMpegPlayer.h"
#ifdef INFINITY_MENU_FFMPEG
#include "XsdkAvPlayer.h"
#include "TvRemoteServer.h"
#include "TvRemoteCatalog.h"
#endif
#include <cerrno>
#include <filesystem>
#include "infinity_menu_preview_shaders.h"
#include <ctime>
#else
#ifdef INFINITY_CONTAINER_OUTPUT_PROBE
#include "cloud360_av_probe_shaders.h"
#include "container_decode.h"
#include "frame_output.h"
#include "av_timeline.h"
#include "XsdkAvPlayer.h"
#ifdef INFINITY_CONTAINER_NETWORK_PROBE
#include "XsdkRemoteReader.h"
#include "hls_vod.h"
#include <fstream>
#endif
#include <chrono>
#include "xsdk/audio.h"
#include "xsdk/xam.h"
#include <algorithm>
#else
#include "infinity_canvas_probe_shaders.h"
#endif
#endif
#include "xsdk/gpu.h"
#include "xsdk/present.h"
#include "xsdk/log.h"
#include <cstring>
#include <cstdio>
#include <vector>
namespace {
constexpr unsigned BG=0xff031310u,LIME=0xff9dec31u,WHITE=0xfff2f7f3u,MUTED=0xffb7c9c1u,PANEL=0xff172c28u;
constexpr unsigned MaxFrames=900, Timeout=1500;
const float quad[][5]={{-1,1,0,0,0},{1,1,0,1,0},{-1,-1,0,0,1},{1,-1,0,1,1}};
struct Presenter {
    XsdkShader vs{},ps{};
    XsdkTexture texture{};
    XsdkSamplerState sampler{};
    volatile uint32_t* vertices=nullptr;
    bool gpu=false,present=false;
    unsigned width=0,height=0,safeArea=0;
    std::vector<unsigned> insetPixels;
    bool Open(){
        xsdk_log_write("Presenter: GPU open");
        if(!NT_SUCCESS(xsdk_gpu_open())){xsdk_log_write("Presenter FAIL: GPU open");return false;}
        gpu=true;
        if(!NT_SUCCESS(xsdk_shader_load(&vs,infinity_canvas_vert.container,infinity_canvas_vert.container_bytes))){xsdk_log_write("Presenter FAIL: vertex shader load");return false;}
        if(!NT_SUCCESS(xsdk_shader_load(&ps,infinity_canvas_frag.container,infinity_canvas_frag.container_bytes))){xsdk_log_write("Presenter FAIL: pixel shader load");return false;}
        XsdkVertexElement layout[2]{};
        layout[0].usage=XSDK_USAGE_POSITION;layout[0].format=XSDK_VERTEX_FLOAT3;
        layout[1].usage=XSDK_USAGE_TEXCOORD;layout[1].format=XSDK_VERTEX_FLOAT2;layout[1].offset_bytes=12;
        const uint32_t stride[]={sizeof quad[0]};
        xsdk_log_write("Presenter: vertex layout POSITION0 + TEXCOORD0 (GLSL locations 0,2)");
        if(!NT_SUCCESS(xsdk_shader_set_vertex_layout(&vs,layout,2,stride))){xsdk_log_write("Presenter FAIL: vertex layout");return false;}
        vertices=xsdk_gpu_alloc(sizeof quad);
        if(!vertices){xsdk_log_write("Presenter FAIL: vertex buffer");return false;}
        for(unsigned i=0;i<sizeof quad/4;++i){uint32_t word;std::memcpy(&word,reinterpret_cast<const char*>(quad)+4*i,4);vertices[i]=word;}
        xsdk_gpu_invalidate(xsdk_gpu_address(vertices),sizeof quad);
        if(!NT_SUCCESS(xsdk_texture_create(&texture,XSDK_FORMAT_A8R8G8B8,HomeCanvas::Width,HomeCanvas::Height,1,true))){xsdk_log_write("Presenter FAIL: canvas texture");return false;}
        XsdkSampler cfg{};cfg.mag_filter=cfg.min_filter=XSDK_FILTER_POINT;
        cfg.address_u=cfg.address_v=XSDK_ADDRESS_CLAMP;
        xsdk_texture_sampler_state(&sampler,&cfg);
        uint32_t w=0,h=0;
        if(!NT_SUCCESS(xsdk_present_open(XSDK_DISPLAY_720P,3,1,&w,&h))){xsdk_log_write("Presenter FAIL: display open");return false;}
        present=true;width=w;height=h;
        xsdk_log_write("Presenter: display %u x %u",width,height);
        if(width!=HomeCanvas::Width || height!=HomeCanvas::Height){xsdk_log_write("Presenter FAIL: unexpected display size");return false;}
        xsdk_log_write("Presenter PASS: shader layout, texture and display initialized");
        return true;
    }
    bool Show(const HomeCanvas& canvas,bool upload){
        // A single texture is reused only after the prior frame has finished.
        if(upload){
            if(!NT_SUCCESS(xsdk_gpu_wait(xsdk_gpu_fence(),Timeout))){xsdk_log_write("Presenter FAIL: frame fence");return false;}
            const unsigned* pixels=DisplaySafeArea::Apply(canvas.Pixels(),safeArea,insetPixels);
            if(!pixels)return false;
            xsdk_texture_upload(&texture,0,pixels,HomeCanvas::Width*4);
            xsdk_gpu_invalidate(texture.address,texture.size_bytes);
        }
        uint32_t address=0;
        if(!NT_SUCCESS(xsdk_present_begin(&address,Timeout))){xsdk_log_write("Presenter FAIL: begin frame");return false;}
        xsdk_gpu_set_target(width,height);
        xsdk_gpu_set_shaders(&vs,&ps);
        xsdk_built_shader_set_literals(&infinity_canvas_vert);
        xsdk_built_shader_set_literals(&infinity_canvas_frag);
        xsdk_gpu_set_texture(INFINITY_CANVAS_FRAG_SAMPLER_0_0,&texture,&sampler);
        xsdk_gpu_set_vertices(0,xsdk_gpu_address(vertices),sizeof quad);
        xsdk_gpu_draw(XSDK_PRIMITIVE_TRIANGLE_STRIP,4);
        xsdk_gpu_resolve(address,width,height,BG);
        xsdk_present_end();
        return true;
    }
    bool Close(){
        if(!gpu)return true;
        const bool idle=NT_SUCCESS(xsdk_gpu_wait(xsdk_gpu_fence(),Timeout));
        if(present)xsdk_present_close();
        // If the fence timed out, let GPU shutdown own recovery; never free
        // buffers that might still be in use.
        if(idle){
            if(texture.texels)xsdk_texture_free(&texture);
            if(vertices)xsdk_gpu_free(vertices);
            if(ps.microcode)xsdk_shader_free(&ps);
            if(vs.microcode)xsdk_shader_free(&vs);
        }
        const bool closed=NT_SUCCESS(xsdk_gpu_close());gpu=false;
        return idle&&closed;
    }
};
#if !defined(INFINITY_FULL_MENU_PREVIEW) && !defined(INFINITY_CONTAINER_OUTPUT_PROBE)
void panel(HomeCanvas& c,int x,int y,int w,int h,bool selected){
    c.Box(x,y,w,h,selected?LIME:0xff365344u,8);
    c.Box(x+3,y+3,w-6,h-6,PANEL,6);
}
void draw(HomeCanvas& c,unsigned page,unsigned focus,size_t row,const std::vector<Game>& games,const TvCatalog& tv){
    c.Background();
    c.Text(44,32,"INFINITY 360",39,LIME,700);
    c.Text(880,49,"XSDK | Canvas Alpha",21,MUTED,350);
    c.Text(45,100,page==0?"Inicio":page==1?"Biblioteca":"Cloud360 IPTV",32,WHITE,1180);
    if(!page){
        const char* names[]={"Biblioteca","Cloud360 IPTV"};
        for(unsigned i=0;i<2;++i){int x=45+int(i)*601;panel(c,x,180,580,340,focus==i);
            c.Text(x+28,215,names[i],36,WHITE,520);
            c.Text(x+28,300,i?"Catalogo M3U local":"Jogos XEX locais",24,MUTED,520);}
        c.Text(45,570,"Teste de renderizacao: sem lancamento ou reproducao",23,MUTED,1180);
    }else{
        size_t count=page==1?games.size():tv.Items().size();
        if(!count)c.Text(48,235,page==1?"Nenhum jogo encontrado em game:/Games":"Lista M3U vazia ou ausente",25,MUTED,1180);
        size_t first=(row/6)*6;
        for(size_t i=first;i<count&&i<first+6;++i){
            int y=165+int(i-first)*65;panel(c,45,y,1188,59,i==row);
            c.Text(65,y+10,page==1?games[i].name:tv.Items()[i].name,23,WHITE,1120);
        }
        c.Text(45,575,"Somente consulta | Y Atualizar | B Inicio",22,MUTED,1180);
    }
    c.Text(45,648,"Direcional Navegar | A Abrir | Back Sair | encerramento automatico",20,LIME,1180);
}
#endif
}
#ifdef INFINITY_CONTAINER_OUTPUT_PROBE
int main(){
    std::string address="game:/test.mp4";
#ifdef INFINITY_CONTAINER_NETWORK_PROBE
    std::ifstream config("game:/cloud360-test-ts-url.txt",std::ios::binary);
    if(config){
        char raw[4098]{};config.getline(raw,sizeof raw);
        if(config.fail()||config.peek()!=std::char_traits<char>::eof())return 1;
        address=raw;if(!address.empty()&&address.back()=='\r')address.pop_back();
    }
#endif
    HomeCanvas canvas;Presenter p;
    if(!p.Open()){p.Close();return 2;}
    auto result=XsdkAv::Play(address,"game:/",canvas,[&](bool upload){return p.Show(canvas,upload);},true);
    const bool closed=p.Close();xsdk_log_write("Cloud360 AV: %s",result.c_str());
    return closed&&(result=="Reproducao encerrada."||result=="Reproducao cancelada.")?0:3;
}
#elif !defined(INFINITY_FULL_MENU_PREVIEW)
int main(){
    HomeCanvas canvas;
    if(!canvas.LoadArt("game:/home.tga")){xsdk_log_write("FAIL: canvas atlas");return 1;}
    auto games=GameScanner(std::vector<std::string>{"game:/Games"}).Scan();
    TvCatalog tv;tv.Load("game:/playlist.m3u");
    Presenter p;
    if(!p.Open()){xsdk_log_write("FAIL: canvas presenter open");p.Close();return 2;}
    unsigned page=0,focus=0;size_t row=0;uint16_t previous=0;
    bool ok=true,dirty=true;
    for(unsigned frame=0;frame<MaxFrames;++frame){
        INPUT_STATE input{};
        uint16_t buttons=XamInputGetState(0,0,&input)==ERROR_SUCCESS?input.gamepad.buttons:0;
        const uint16_t edge=buttons&~previous;previous=buttons;
        if(edge&INPUT_GAMEPAD_BACK)break;
        if(edge&INPUT_GAMEPAD_B){if(!page)break;page=0;row=0;dirty=true;}
        else if(!page){
            if(edge&INPUT_GAMEPAD_DPAD_LEFT){focus=0;dirty=true;}
            if(edge&INPUT_GAMEPAD_DPAD_RIGHT){focus=1;dirty=true;}
            if(edge&INPUT_GAMEPAD_A){page=focus+1;row=0;dirty=true;}
        }else{
            size_t count=page==1?games.size():tv.Items().size();
            if(edge&INPUT_GAMEPAD_DPAD_UP && row){--row;dirty=true;}
            if(edge&INPUT_GAMEPAD_DPAD_DOWN && row+1<count){++row;dirty=true;}
            if(edge&INPUT_GAMEPAD_Y){if(page==1)games=GameScanner(std::vector<std::string>{"game:/Games"}).Scan();else tv.Load("game:/playlist.m3u");row=0;dirty=true;}
        }
        if(dirty)draw(canvas,page,focus,row,games,tv);
        if(!p.Show(canvas,dirty)){ok=false;break;}
        dirty=false;
    }
    const bool restored=p.Close();
    xsdk_log_write("%s: canvas GPU return",ok&&restored?"PASS":"FAIL");
    return ok&&restored?0:3;
}

#else
int main(){
    HomeCanvas canvas;
    if(!canvas.LoadArt("game:/home.tga")){xsdk_log_write("FAIL: menu atlas");return 1;}
    // Require an isolated test-data directory. Never use the live dashboard profile.
    const std::string dir="game:/MenuPreview/";
    std::ifstream marker((dir+"infinity-preview.txt").c_str());
    std::string signature;
    if(!std::getline(marker,signature)||signature!="INFINITY360-XSDK-PREVIEW"){
        xsdk_log_write("FAIL: isolated MenuPreview marker missing");return 2;
    }
    ConsoleHome menu(dir,GameScanner::DefaultRoots());menu.Load();
    menu.SetAvatarCapabilities(InfinitySocial::ProbeAvatarExports());
    std::ifstream launchMarker((dir+"enable-xex-launch.txt").c_str());
    std::string launchSignature;
    const bool launchEnabled=bool(std::getline(launchMarker,launchSignature)) && launchSignature=="ENABLE-XEX-LAUNCH";
    launchMarker.close();
    std::string pendingLaunch,launchNotice;
    XexLaunchCheck::Ticket launchTicket{};
    PackageLaunch::Ticket packageTicket{};bool pendingPackage=false;
    std::ifstream packageMarker((dir+"enable-package-launch.txt").c_str());std::string packageSignature;
    const bool packageEnabled=bool(std::getline(packageMarker,packageSignature))&&packageSignature=="ENABLE-PACKAGE-LAUNCH";packageMarker.close();
    std::ifstream sessionMarker((dir+"enable-continuous-session.txt").c_str());std::string sessionSignature;
    const bool continuous=bool(std::getline(sessionMarker,sessionSignature))&&sessionSignature=="ENABLE-CONTINUOUS-SESSION";sessionMarker.close();
    auto captureLaunch=[](const std::string& path,XexLaunchCheck::Ticket& ticket){
        std::error_code ec;if(path.empty()||!std::filesystem::is_regular_file(path,ec)||ec)return false;
        XboxExecutionInfo info{};if(ContentCatalog::ReadExecutionInfo(path,info)&&XexLaunchCheck::OwnTitle(info.titleId))return false;
        std::ifstream input(path.c_str(),std::ios::binary);return XexLaunchCheck::Capture(input,ticket);
    };
    Presenter p;
    if(!p.Open()){xsdk_log_write("FAIL: menu presenter open");p.Close();return 3;}
    p.safeArea=menu.SafeArea();
    const uint16_t keys[]={INPUT_GAMEPAD_DPAD_LEFT,INPUT_GAMEPAD_DPAD_RIGHT,
        INPUT_GAMEPAD_DPAD_UP,INPUT_GAMEPAD_DPAD_DOWN,INPUT_GAMEPAD_A,
        INPUT_GAMEPAD_B,INPUT_GAMEPAD_X,INPUT_GAMEPAD_Y,0x0100,0x0200,
        INPUT_GAMEPAD_START,INPUT_GAMEPAD_BACK};
    uint16_t previous=0;
    bool ok=true,dirty=true,systemExit=false;
    DashboardMotion motion;
    FileWorkspace captureWorkspace(dir);
    IdleScreen idleScreen;
    InfinitySocial::XsdkSocialListener socialListener(dir);
#ifdef INFINITY_MENU_FFMPEG
    TvRemote::Server dashboardRemote;
    const bool dashboardRemoteEnabled=TvPreferences::Read(dir+"enable-dashboard-remote.txt")=="ENABLE-DASHBOARD-REMOTE";
    uint64_t remoteRetry=0;
#endif
    // Longer than the minimal GPU probe, but still bounded.
    std::string clock;
    for(unsigned frame=0;continuous||frame<3600;++frame){
        if(frame%60==0){const auto value=menu.ClockLabel(std::time(nullptr));if(clock!=value){clock=value;dirty=true;}}
        INPUT_STATE input{};
        uint16_t buttons=XamInputGetState(0,0,&input)==ERROR_SUCCESS?input.gamepad.buttons:0;
        if(input.gamepad.thumb_lx < -16000)buttons|=INPUT_GAMEPAD_DPAD_LEFT;
        if(input.gamepad.thumb_lx > 16000)buttons|=INPUT_GAMEPAD_DPAD_RIGHT;
        if(input.gamepad.thumb_ly > 16000)buttons|=INPUT_GAMEPAD_DPAD_UP;
        if(input.gamepad.thumb_ly < -16000)buttons|=INPUT_GAMEPAD_DPAD_DOWN;
        uint16_t edge=buttons&~previous;previous=buttons;
        const bool systemUiOpen=xsdk_system_ui_open();
        if(systemUiOpen)edge=0;
        bool remoteStoreJob=false;
#ifdef INFINITY_MENU_FFMPEG
        if(dashboardRemoteEnabled){
            const auto now=TransferRate::Now();
            if(dashboardRemote.Address().empty()&&(!remoteRetry||now>=remoteRetry)){if(dashboardRemote.Start(dir))menu.LoadRemoteStore();remoteRetry=now+5000;}
            ConsoleHome::Key remoteKey=ConsoleHome::Back;
            const auto command=dashboardRemote.Poll([&](TvRemote::Key cmd){return !systemUiOpen&&!buttons&&pendingLaunch.empty()&&ConsoleHome::RemoteKey(cmd,remoteKey)&&menu.RemoteAllowed(remoteKey);},[&](){return menu.RemoteSnapshot();},[&](const TvRemote::Request& request){return menu.RemoteCatalog(request);},{},[&](const TvRemote::Request& request){return !systemUiOpen&&!buttons&&!idleScreen.Active()&&pendingLaunch.empty()&&menu.RemoteStoreAction(request);});
            if(command==TvRemote::Key::StoreDownload||command==TvRemote::Key::StoreStart){remoteStoreJob=true;dirty=true;}
            else if(command==TvRemote::Key::StoreQueue||command==TvRemote::Key::StoreRemove){dirty=true;}
            else if(command!=TvRemote::Key::None){
                if(remoteKey==ConsoleHome::GlobalGuide)edge|=INPUT_GAMEPAD_LEFT_THUMB;
                else if(static_cast<unsigned>(remoteKey)<11)edge|=keys[static_cast<unsigned>(remoteKey)];
            }
        }
#endif
        const bool wasIdle=idleScreen.Active();
        const bool activity=remoteStoreJob||buttons||edge||input.gamepad.left_trigger>30||input.gamepad.right_trigger>30||input.gamepad.thumb_rx>16000||input.gamepad.thumb_rx< -16000||input.gamepad.thumb_ry>16000||input.gamepad.thumb_ry< -16000;
        if(idleScreen.Update(IdleScreen::Now(),menu.IdleMinutes(),continuous&&pendingLaunch.empty()&&menu.IdleEligible(),activity))dirty=true;
        if(wasIdle&&activity)edge=0;
        const uint16_t captureChord=INPUT_GAMEPAD_LEFT_THUMB|INPUT_GAMEPAD_RIGHT_THUMB;
        const bool capture=!systemUiOpen&&(buttons&captureChord)==captureChord&&(edge&captureChord)!=0;
        if(capture)edge=0;
        const bool guideEdge=!systemUiOpen&&pendingLaunch.empty()&&(edge&INPUT_GAMEPAD_LEFT_THUMB);
        if(guideEdge){
            menu.Press(ConsoleHome::GlobalGuide);edge=0;dirty=true;
        }
        bool uiInput=guideEdge;for(const auto key:keys)if(edge&key)uiInput=true;
        // BACK opens contextual options; R3 alone is not a command.
        if(!pendingLaunch.empty()){
            if(edge&INPUT_GAMEPAD_B){pendingLaunch.clear();launchNotice="Lancamento cancelado.";dirty=true;}
            else if(edge&INPUT_GAMEPAD_A){
                const auto path=pendingLaunch;pendingLaunch.clear();
                if(pendingPackage){
                    pendingPackage=false;XsdkPackageMount mount;std::string error;
                    menu.StopFtp();
                    if(menu.TransferRunning()||!mount.Open(path,packageTicket,error)){launchNotice=error.empty()?"Transferencia ativa.":error;dirty=true;}
                    else{
                        if(!p.Close()){mount.Close();ok=false;break;}
                        PackageLaunch::Ticket last{};
                        if(PackageLaunch::Capture(path,last)&&PackageLaunch::Same(packageTicket,last)){
                            menu.RecordLaunchAttempt(path);
                            // Successful xsdk_launch terminates the title; on early failure close the mount.
                            (void)xsdk_launch(XsdkPackageMount::Executable,nullptr,0);
                            menu.RecordLaunchFailure(path);
                            launchNotice="Loader retornou erro; montagem encerrada.";
                        }else launchNotice="Pacote mudou antes do lancamento.";
                        mount.Close();p=Presenter{};if(!p.Open()){p.Close();ok=false;break;}dirty=true;
                    }
                }else{
                XexLaunchCheck::Ticket current{};
                if(menu.TransferRunning()||!captureLaunch(path,current)||!XexLaunchCheck::Same(launchTicket,current)){
                    launchNotice="Executavel mudou ou copia ativa. Selecione novamente.";dirty=true;
                }else{
                    menu.StopFtp();
                    if(!p.Close()){ok=false;break;}
                    XexLaunchCheck::Ticket last{};
                    if(captureLaunch(path,last)&&XexLaunchCheck::Same(launchTicket,last)){
                        menu.RecordLaunchAttempt(path);
                        xsdk_log_write("XEX launch confirmed");
                        (void)xsdk_launch(path.c_str(),nullptr,0);
                        menu.RecordLaunchFailure(path);
                        xsdk_log_write("XEX launch returned errno=%d",errno);
                        launchNotice="O console recusou o executavel. Retornando ao menu.";
                    }else launchNotice="Midia alterada antes do lancamento.";
                    p=Presenter{};
                    if(!p.Open()){p.Close();ok=false;break;}
                    dirty=true;
                }
                }
            }
        }else for(unsigned i=0;i<=sizeof keys/sizeof keys[0];++i)if(i==sizeof keys/sizeof keys[0]?remoteStoreJob:!remoteStoreJob&&(edge&keys[i])){
            launchNotice.clear();
            const std::string launch=remoteStoreJob?std::string{}:menu.Press(i==11?ConsoleHome::Context:static_cast<ConsoleHome::Key>(i));
            if(!launch.empty()){
                const std::string path=XexLaunchCheck::Path(launch);
                pendingPackage=false;
                if(!path.empty()){
                    if(!launchEnabled)launchNotice="XEX bloqueado: habilite a execução nas configurações.";
                    else if(!captureLaunch(path,launchTicket))launchNotice="XEX invalido, indisponivel ou da propria dashboard.";
                    else pendingLaunch=path;
                }else{
                    const auto pkg=XsdkPaths::Normalize(launch);
                    if(!packageEnabled||!launchEnabled)launchNotice="Habilite XEX e montagem de pacotes em MenuPreview.";
                    else if(pkg.empty()||!PackageLaunch::Capture(pkg,packageTicket))launchNotice="GOD/XBLA invalido, incompleto ou layout nao suportado.";
                    else {pendingPackage=true;pendingLaunch=pkg;}
                }
            }
            auto movie=menu.TakeMovie();
            if(!movie.empty()){
                menu.StopFtp();bool presentFailed=false;
                canvas.Box(0,0,1280,720,0xff000000u);
                canvas.Text(45,80,"Cloud360 | Abrindo midia",32,WHITE,1180);
                if(!p.Show(canvas,true)){ok=false;break;}
                auto presentMovie=[&](bool upload){if(!p.Show(canvas,upload)){presentFailed=true;return false;}return true;};
                auto drawGuide=[&](HomeCanvas& frame,int row){menu.DrawPlaybackGuide(frame,row);};
#ifdef INFINITY_MENU_FFMPEG
                std::string result;
                TvRemote::Server playbackRemote;
                auto& remoteControl=dashboardRemoteEnabled?dashboardRemote:playbackRemote;
                MediaUrl remoteUrl;
                if(remoteControl.Address().empty()&&remoteUrl.Parse(movie)&&XsdkAv::Supports(movie))remoteControl.Start(dir);
                const auto initialAudio=TvPreferences::LoadAudio(dir);
                CloudPlaybackControls::AudioPreference channelAudio{initialAudio.volume,initialAudio.muted};
                do{
                    std::string next;
                    std::function<bool(int)> zap;
                    if(menu.CanZapTv())zap=[&](int direction){next=menu.ZapTv(direction);return !next.empty();};
                    result=XsdkAv::Supports(movie)?XsdkAv::Play(movie,dir,canvas,presentMovie,false,drawGuide,zap,&channelAudio,[&](){menu.RecordTvPlayed();},[&](int64_t now){return menu.PlayingTvInfo(now);},[&](){menu.ToggleTvPlayingFavorite();},[&](){return menu.TvChannelChoices();},[&](size_t row){next=menu.ChooseTvChannel(row);return !next.empty();},menu.PlayingTvLogo(),[&](bool allowTune){return remoteControl.Poll([](TvRemote::Key key){return XsdkAv::RemoteButtons(key)!=0;},[&](){return menu.RemoteSnapshot(true);},[&](const TvRemote::Request& request){return menu.RemoteCatalog(request);},[&](const TvRemote::Request& request){if(!allowTune)return false;next=menu.ChooseRemoteTvChannel(request.query,[](const std::string& path){return XsdkAv::Supports(path);});return !next.empty();});},remoteControl.Address()):XsdkMpeg::Play(movie,dir,canvas,presentMovie,drawGuide);
                    if(result!="Alterando canal."||next.empty()||presentFailed)break;
                    movie=next;
                }while(!XsdkAv::Cancelled());
                if((channelAudio.volume!=initialAudio.volume||channelAudio.muted!=initialAudio.muted)&&
                    !TvPreferences::SaveAudio(dir,{channelAudio.volume,channelAudio.muted}))
                    result+=" | Falha ao salvar audio.";
#else
                const auto result=XsdkMpeg::Play(movie,dir,canvas,presentMovie,drawGuide);
#endif
                menu.FinishTvPlayback();menu.PlayerMessage(result);dirty=true;
                if(presentFailed){ok=false;break;}
                INPUT_STATE held{};previous=XamInputGetState(0,0,&held)==ERROR_SUCCESS?held.gamepad.buttons:0;
            }
            StoreItem pending;
#ifdef INFINITY_XSDK_HTTPS
            if(menu.TakeDiscSystemExit()){systemExit=true;break;}
            unsigned artTitle=0;bool artCover=false,artRemote=false;
            XboxContentPackage backup{};ContentBackup::Record recovery;
            std::vector<unsigned> coverBatch;
            if(menu.TakeCoverBatch(coverBatch)){
                menu.StopFtp();unsigned saved=0,failed=0;bool cancelled=false,videoFailed=false;
                for(size_t index=0;index<coverBatch.size()&&!cancelled;++index){
                    auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                    auto progress=[&](unsigned long long done,unsigned long long total){
                        INPUT_STATE state{};if(XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK))){cancelled=true;return false;}
                        const auto now=std::chrono::steady_clock::now();if(done!=total&&now-last<std::chrono::milliseconds(200))return true;last=now;
                        canvas.Background();canvas.Text(48,100,"XboxUnity | Capas em lote",36,WHITE,1180);
                        canvas.Text(48,220,std::to_string(index+1)+" / "+std::to_string(coverBatch.size())+" | Salvas "+std::to_string(saved),28,WHITE,1180);
                        canvas.Text(48,300,StorageInfo::FormatBytes(done)+" recebidos",24,MUTED,1180);
                        canvas.Text(48,560,"B Cancelar | Capas existentes preservadas",22,LIME,1180);
                        if(!p.Show(canvas,true)){videoFailed=true;cancelled=true;return false;}return true;
                    };
                    if(!progress(0,1))break;
                    bool success=false;const auto result=XsdkUnityCover(coverBatch[index],dir,progress,success);
                    if(success)++saved;else if(!cancelled)++failed;
                    // Stop after an initial failure to avoid repeatedly querying an unavailable provider.
                    if(index==0&&!success&&!cancelled)break;
                }
                menu.CoverBatchResult(saved,failed,cancelled);
                if(videoFailed){ok=false;break;}
            }
            if(menu.TakeArtRequest(artTitle,artCover,artRemote)){
                menu.StopFtp();bool success=false,videoFailed=false;std::string result;
                auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};if(XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();if(done!=total&&now-last<std::chrono::milliseconds(200))return true;last=now;
                    canvas.Background();canvas.Text(48,100,"INFINITY | Importar arte",36,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Text(48,560,"B Cancelar | Sem sobrescrita",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}return true;
                };
                if(artRemote)result=XsdkUnityCover(artTitle,dir,progress,success);
                else{FileWorkspace ws(dir);success=ArtImport::Local(ws,dir,artTitle,artCover,result);if(success)result="Arte local importada.";}
                menu.ArtResult(result,success);if(videoFailed){ok=false;break;}
            }
            ContentInstall::Request contentInstall;XboxExecutionInfo contentGame{};
            if(menu.TakeContentInstall(contentInstall,contentGame)){
                menu.StopFtp();bool videoFailed=false;auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};if(XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();if(done!=total&&now-last<std::chrono::milliseconds(200))return true;last=now;
                    canvas.Background();canvas.Text(48,100,"INFINITY | Instalar TU/DLC",36,WHITE,1180);
                    canvas.Text(48,210,contentInstall.filename,24,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    canvas.Text(48,560,"B Cancelar | SHA-256 | Sem sobrescrita",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}return true;
                };
                FileWorkspace ws(dir);std::string result;const bool success=ContentInstall::Install(ws,dir,contentGame,contentInstall,progress,result);
                menu.ContentInstallResult(success?"Conteudo copiado para HDD; reconhecimento ainda nao confirmado.":result,success);
                if(videoFailed){ok=false;break;}
            }
            bool verifyOnly=false;
            const bool recover=menu.TakeRecoverRequest(recovery,verifyOnly);
            if(recover||menu.TakeBackupRequest(backup)){
                menu.StopFtp();bool videoFailed=false;auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};if(XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();if(done!=total&&now-last<std::chrono::milliseconds(200))return true;last=now;
                    canvas.Background();canvas.Text(48,100,recover?(verifyOnly?"INFINITY | Verificar backup":"INFINITY | Recuperar backup"):"INFINITY | Backup de conteudo",36,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    canvas.Text(48,560,"B Cancelar | Origem intacta | SHA-256",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}return true;
                };
                FileWorkspace ws(dir);std::string result;
                const bool success=recover?(verifyOnly?ContentBackup::Verify(ws,dir,recovery,progress,result):ContentBackup::Recover(ws,dir,recovery,progress,result)):ContentBackup::Archive(ws,dir,backup,progress,result);
                if(recover)menu.RecoverResult(success?(verifyOnly?"SHA-256 do backup confere; nenhum arquivo escrito.":"Copia verificada em area pessoal; sem ativacao."):result,success);
                else menu.BackupResult(success?"Backup verificado em UserFiles/ContentBackups.":result,success);
                if(videoFailed){ok=false;break;}
            }
            XboxExecutionInfo unityInfo{};
            if(menu.TakeUnityRequest(unityInfo)){
                menu.StopFtp();bool success=false,videoFailed=false;
                auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                const auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};
                    if(XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();
                    if(done!=total&&now-last<std::chrono::milliseconds(200))return true;
                    last=now;canvas.Background();
                    canvas.Text(48,100,"XboxUnity | Consultar Title Updates",36,WHITE,1180);
                    canvas.Text(48,210,"Consultando metadados por HTTPS...",28,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" recebidos",24,MUTED,1180);
                    canvas.Text(48,560,"B Cancelar | Sem download ou ativacao de TU",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}return true;
                };
                std::vector<UnityUpdate> updates;
                const auto result=XsdkUnityUpdates(unityInfo,dir,progress,updates,success);
                menu.UnityResult(updates,result,success);
                if(videoFailed){ok=false;break;}
            }
            UnityUpdate selectedUpdate;
            if(menu.TakeUnityDownload(unityInfo,selectedUpdate)){
                menu.StopFtp();bool success=false,videoFailed=false;
                auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                const auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};
                    if(XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();
                    if(done!=total&&now-last<std::chrono::milliseconds(200))return true;
                    last=now;canvas.Background();
                    canvas.Text(48,100,"XboxUnity | Download de TU",36,WHITE,1180);
                    canvas.Text(48,210,"TU ID "+std::to_string(selectedUpdate.id),28,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);
                    if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    canvas.Text(48,560,"B Cancelar | MD5 + Title ID + Media ID | Sem ativacao",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}return true;
                };
                const auto result=XsdkUnityDownload(unityInfo,selectedUpdate,dir,progress,success);
                menu.UnityDownloadResult(result,success);
                if(videoFailed){ok=false;break;}
            }
            InfinityLuaRepo::Manifest luaManifest;
            if(menu.TakeLuaManifest(luaManifest)){
                menu.StopFtp();bool success=false,videoFailed=false;
                auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                const auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};
                    if(XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();
                    if(done!=total&&now-last<std::chrono::milliseconds(200))return true;
                    last=now;canvas.Background();
                    canvas.Text(48,100,"INFINITY Lua | Sincronizar catalogo",36,WHITE,1180);
                    canvas.Text(48,210,"Manifesto fixado",28,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);
                    if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    canvas.Text(48,560,"B Cancelar | TLS e SHA-256 | Sem instalar scripts",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}return true;
                };
                const auto result=XsdkLuaSync(luaManifest,dir,progress,success);
                menu.LuaManifestResult(result,success);
                if(videoFailed){ok=false;break;}
            }
            InfinityLuaRepo::Entry script;
            if(menu.TakeLuaDownload(script)){
                menu.StopFtp();bool success=false,videoFailed=false;
                auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                const auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};
                    if(XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();
                    if(done!=total&&now-last<std::chrono::milliseconds(200))return true;
                    last=now;canvas.Background();
                    canvas.Text(48,100,"INFINITY Lua | Download",36,WHITE,1180);
                    canvas.Text(48,210,script.file,28,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);
                    if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    canvas.Text(48,560,"B Cancelar | TLS e SHA-256 | Sem execucao",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}return true;
                };
                const auto result=XsdkLuaDownload(script,dir,progress,success);
                menu.LuaDownloadResult(result,success);
                if(videoFailed){ok=false;break;}
            }
            StoreProvider provider;
            if(menu.TakeProviderRequest(provider)){
                menu.StopFtp();bool success=false,videoFailed=false;
                auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                const auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};
                    if(XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();
                    if(done!=total&&now-last<std::chrono::milliseconds(200))return true;
                    last=now;canvas.Background();
                    canvas.Text(48,100,"INFINITY Store | Sincronizar catalogo",36,WHITE,1180);
                    canvas.Text(48,210,provider.name,28,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);
                    if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    canvas.Text(48,560,"B Cancelar | TLS e SHA-256 | Nenhum pacote iniciado",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}return true;
                };
                const auto result=XsdkSyncProvider(provider,dir,progress,success);
                menu.ProviderResult(result,success);
                if(videoFailed){ok=false;break;}
            }
            PackageLaunch::Ticket importTicket;
            if(menu.TakePackageImport(importTicket)){
                menu.StopFtp();FileWorkspace workspace(dir);bool videoFailed=false;
                auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};if(XamInputGetState(0,0,&state)==ERROR_SUCCESS&&(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();if(done!=total&&now-last<std::chrono::milliseconds(200))return true;last=now;
                    canvas.Background();canvas.Text(48,100,"INFINITY | Importar GOD/XBLA",36,WHITE,1180);
                    canvas.Text(48,210,importTicket.package.name,24,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    canvas.Text(48,560,"B Cancelar | SHA-256 de cada arquivo | Original intacto",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}return true;
                };
                std::string error;const bool success=PackageImport::Install(workspace,dir,importTicket,progress,error);
                menu.PackageImportResult(success?"Pacote importado; consulte Biblioteca.":error,success);
                if(videoFailed){ok=false;break;}
            }
            DiscInstallRequest disc;
            if(menu.TakeDiscInstallRequest(disc)){
                const bool god=disc.format==DiscInstallRequest::Format::God;
                menu.StopFtp();FileWorkspace workspace(dir);
                bool videoFailed=false;
                auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                const auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};
                    if(XamInputGetState(0,0,&state)==ERROR_SUCCESS &&
                       (state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();
                    if(now-last<std::chrono::milliseconds(200))return true;
                    last=now;canvas.Background();
                    canvas.Text(48,100,god?"INFINITY 360 | DVD para GOD":"INFINITY 360 | Extrair DVD",36,WHITE,1180);
                    canvas.Text(48,210,"Destino: "+disc.mount,28,WHITE,1180);
                    const auto half=total/2;
                    const std::string phase=half?(done>=half?"Verificando ":"Gravando "):"Lendo estrutura do DVD";
                    canvas.Text(48,300,half?phase+StorageInfo::FormatBytes(done>=half?done-half:done)+" / "+StorageInfo::FormatBytes(half):phase,24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);
                    if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    canvas.Text(48,560,god?"B Cancelar | Fragmentos GOD e verificacao SHA-1":"B Cancelar | Copia e verificacao de leitura",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}
                    return true;
                };
                std::string installed,error;
                const bool success=god?InstallDiscGod(workspace,dir,"dvd:/",disc.mount,progress,installed,error,disc.identity):
                    InstallDiscFiles(workspace,dir,"dvd:/",disc.mount,progress,installed,error,&disc.identity);
                menu.DiscInstallResult(success?(god?"GOD verificado na biblioteca de "+disc.mount:"DVD extraido e verificado em "+disc.mount):error,success);
                if(videoFailed){ok=false;break;}
            }
            const bool dashboardUpdate=menu.TakeDashboardUpdate(pending);
            if(dashboardUpdate||menu.TakeStoreInstallRequest(pending)){
                menu.StopFtp();
                FileWorkspace workspace(dir);
                bool videoFailed=false;
                auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                const auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};
                    if(XamInputGetState(0,0,&state)==ERROR_SUCCESS &&
                       (state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();
                    if(done!=total&&now-last<std::chrono::milliseconds(200))return true;
                    last=now;canvas.Background();
                    canvas.Text(48,100,dashboardUpdate?"INFINITY | Instalar nova versao":"INFINITY Store | Instalar pacote",36,WHITE,1180);
                    canvas.Text(48,210,pending.name,28,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);
                    if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    canvas.Text(48,560,"B Cancelar | Nenhuma execucao automatica",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}
                    return true;
                };
                std::string error;
#ifdef INFINITY_STORE_ZIP
                const bool success=StoreInstallManaged(workspace,pending,dir,progress,error);
#else
                const bool success=StoreInstallXex(workspace,pending,dir,progress,error);
#endif
                if(dashboardUpdate)menu.DashboardUpdateResult(success?"Candidata instalada. Inicie default.xex por gerenciador externo.":error,success);
                else menu.StoreInstallResult(success?"Pacote instalado. Abra Apps, Homebrew ou Emuladores.":error,success);
                if(videoFailed){ok=false;break;}
            }
#endif
            while(menu.TakeNativeStoreRequest(pending)){
#ifdef INFINITY_XSDK_HTTPS
                menu.StopFtp();
                bool success=false;
                bool videoFailed=false;
                const auto started=std::chrono::steady_clock::now();
                TransferRate rate;
                auto last=started-std::chrono::seconds(1);
                const auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};
                    if(XamInputGetState(0,0,&state)==ERROR_SUCCESS &&
                       (state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    rate.Sample(done,total,TransferRate::Now());
#ifdef INFINITY_MENU_FFMPEG
                    if(dashboardRemoteEnabled){
                        const auto command=dashboardRemote.Poll([](TvRemote::Key){return false;},{},[&](const TvRemote::Request& request){
                            if(request.key!=TvRemote::Key::Downloads)return std::string("{}");
                            auto body=menu.RemoteCatalog(request);if(body.empty()||body.back()!='}')return std::string("{}");
                            body.pop_back();return body+",\"active_name\":"+TvRemote::Quote(pending.name)+",\"active_done\":"+std::to_string(done)+",\"active_total\":"+std::to_string(total)+"}";
                        },{},[&](const TvRemote::Request& request){return request.key==TvRemote::Key::StorePause&&menu.RemoteStoreEnabled()&&done<total;});
                        if(command==TvRemote::Key::StorePause)return false;
                    }
#endif
                    const auto now=std::chrono::steady_clock::now();
                    if(done!=total&&now-last<std::chrono::milliseconds(200))return true;
                    last=now;
                    canvas.Background();
                    canvas.Text(48,100,"INFINITY Store | HTTPS",36,WHITE,1180);
                    canvas.Text(48,210,pending.name,28,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);
                    if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    canvas.Text(48,430,rate.Label(done,total),22,MUTED,1180);
                    canvas.Text(48,560,"B Pausar | TLS 1.2 | SHA-256 antes de salvar",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}
                    return true;
                };
                const std::string result=XsdkDownload(pending,dir,progress,success);
                menu.NativeStoreResult(result,success);
                if(videoFailed){ok=false;break;}
                // A successful batch item schedules the next; a paused or failed one stops the batch.
#else
                menu.NativeStoreResult("HTTPS ainda indisponivel no XSDK.",false);
#endif
            }
            dirty=true;break;
        }
        if(!ok||systemExit)break;
#ifdef INFINITY_XSDK_HTTPS
        if(pendingLaunch.empty()&&!idleScreen.Active()){
            menu.WeatherAutoDue(std::time(nullptr));
            unsigned weatherCity=0;
            if(!menu.TransferRunning()&&menu.TakeWeatherRequest(weatherCity)){
                ClockWeather::Reading reading;
                auto progress=[&](unsigned long long,unsigned long long){INPUT_STATE state{};return XamInputGetState(0,0,&state)!=ERROR_SUCCESS||!(state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK));};
                auto message=XsdkWeather(weatherCity,dir,progress,reading);menu.WeatherResult(reading,message);dirty=true;
            }
        }
#endif
        if(menu.TakeSigninRequest()){
            const bool allowed=!systemUiOpen&&pendingLaunch.empty()&&!menu.TransferRunning();
            const bool accepted=allowed&&XamShowSigninUI(4,0)==ERROR_SUCCESS;
            menu.SigninResult(accepted);dirty=true;
        }
        const auto friendNow=TransferRate::Now();
        // Read the independent opt-in once per dashboard session, not per frame.
        static const bool autoFriendPresence=[&](){
            std::ifstream flag(dir+"enable-native-friend-auto.txt");
            std::string line;
            return std::getline(flag,line)&&(line=="ENABLE-NATIVE-FRIEND-AUTO"||line=="ENABLE-NATIVE-FRIEND-AUTO\r");
        }();
        const bool friendAllowed=!systemUiOpen&&pendingLaunch.empty()&&!menu.TransferRunning()&&!idleScreen.Active();
        if(menu.UpdateFriendPresence(friendNow,autoFriendPresence,friendAllowed))dirty=true;
        if(menu.TakeFriendRosterRequest()){
            InfinitySocial::FriendRoster roster;
            const bool allowed=!systemUiOpen&&pendingLaunch.empty()&&!menu.TransferRunning()&&!idleScreen.Active();
            const auto result=allowed?InfinitySocial::ReadNativeFriendRoster(menu.SocialHeader(),dir,roster):
                std::string("Consulta bloqueada por outra operacao.");
            menu.FriendRosterResult(roster,result,TransferRate::Now());dirty=true;
        }
        const bool manualAvatar=menu.TakeAvatarBodyRequest();
        static const bool autoAvatar=[&](){
            std::ifstream flag(dir+"enable-native-avatar-auto.txt");std::string line;
            return std::getline(flag,line)&&(line=="ENABLE-NATIVE-AVATAR-AUTO"||line=="ENABLE-NATIVE-AVATAR-AUTO\r");
        }();
        const bool avatarAllowed=!systemUiOpen&&pendingLaunch.empty()&&!menu.TransferRunning()&&!idleScreen.Active();
        if(manualAvatar||(autoAvatar&&avatarAllowed&&menu.AvatarAutoDue(TransferRate::Now()))){
            InfinitySocial::AvatarPreview preview;
            const auto before=menu.SocialHeader();
            const auto result=avatarAllowed?InfinitySocial::ReadNativeAvatarBody(before,dir,preview):
                std::string("Consulta bloqueada por outra operacao.");
            menu.AvatarBodyResult(preview,result,TransferRate::Now(),manualAvatar);
            if(manualAvatar&&avatarAllowed){
                menu.AvatarManifestResult(before,InfinitySocial::ReadNativeAvatarManifest(before,dir));
                menu.AvatarBudgetResult(before,InfinitySocial::ReadNativeAvatarBudget(before,dir));
            }
            dirty=true;
        }
        if(menu.ExpirePrivileges(TransferRate::Now()))dirty=true;
        if(menu.TakePrivilegeRequest()){
            InfinitySocial::ProfilePrivileges privileges;
            const bool allowed=!systemUiOpen&&pendingLaunch.empty()&&!menu.TransferRunning()&&!idleScreen.Active();
            const auto result=allowed?InfinitySocial::ReadNativePrivileges(menu.SocialHeader(),dir,privileges,TransferRate::Now()):
                std::string("Consulta bloqueada por outra operacao.");
            menu.PrivilegeResult(privileges,result);dirty=true;
        }
        unsigned socialTitle=0;std::uint64_t socialTarget=0;
        const auto socialAction=menu.TakeNativeSocialRequest(&socialTitle,&socialTarget);
        if(socialAction!=InfinitySocial::NativeAction::None){
            const bool allowed=!systemUiOpen&&pendingLaunch.empty()&&!menu.TransferRunning()&&!idleScreen.Active()&&
                (socialAction!=InfinitySocial::NativeAction::AvatarEditor||!menu.AvatarLaunchBusy());
            menu.NativeSocialResult(allowed?InfinitySocial::OpenNativeSocialUI(socialAction,menu.SocialHeader(),dir,socialTitle,socialTarget):
                "Interface social bloqueada por outra operacao.");
            dirty=true;
        }
        if(menu.Tick())dirty=true;
        if(socialListener.Poll(menu.SocialHeader(),TransferRate::Now(),pendingLaunch.empty()&&!menu.TransferRunning(),
            [&](const char* label){menu.NativeSocialSignal(label);}))dirty=true;
        if(p.safeArea!=menu.SafeArea()){p.safeArea=menu.SafeArea();dirty=true;}
        const auto motionNow=TransferRate::Now();
        const bool animate=menu.AnimationsEnabled()&&!idleScreen.Active();
        motion.Observe(motionNow,menu.VisualScene(),uiInput,animate);
        const auto animation=motion.Sample(motionNow,animate);
        canvas.SetFocusAnimation(animation.focus);canvas.SetSceneAnimation(animation.scene);
        if(animation.redraw||uiInput)dirty=true;
        if(dirty&&idleScreen.Active())IdleScreen::Draw(canvas,idleScreen.Step());
        else if(dirty){
            menu.Draw(canvas,clock.empty()?"--:--":clock);
            if(!menu.ApprovedHeader()){canvas.Text(920,678,"INFINITY 360",16,LIME,300);}

            if(!launchNotice.empty()){
                canvas.NoticeSurface(40,590,1200,38);
                canvas.Text(54,593,launchNotice,22,LIME,1160);
            }
            if(!pendingLaunch.empty()){
                canvas.Box(180,220,920,260,LIME,12);
                canvas.Box(184,224,912,252,0xff102923u,10);
                canvas.Text(210,242,pendingPackage?"Montar e executar GOD/XBLA?":"Executar XEX selecionado?",32,WHITE,850);
                canvas.Text(210,310,pendingLaunch,20,MUTED,850);
                canvas.Text(210,392,"A Confirmar | B Cancelar | retorna pela dashboard do console",20,LIME,850);
            }
        }
        if(!p.Show(canvas,dirty)){ok=false;break;}
        dirty=false;
        if(capture&&pendingLaunch.empty()){
            if(menu.TransferRunning())launchNotice="Conclua as transferencias antes de capturar.";
            else{
                menu.StopFtp();
                const auto saved=DashboardScreenshot::Save(canvas,captureWorkspace,dir);
                launchNotice=saved.empty()?"Captura recusada: confira UserFiles e enable-file-writes.txt.":"Captura salva: "+saved.substr(saved.find_last_of("/\\")+1);
            }
            dirty=true;
        }
    }
    menu.StopFtp();
    const bool restored=p.Close();
    xsdk_log_write("%s: full menu preview GPU return",ok&&restored?"PASS":"FAIL");
    if(systemExit){
        xsdk_log_write("NXE handoff requested; normal title exit, not automatic installation.");
        return ok&&restored?0:4;
    }
    return ok&&restored?0:4;
}
#endif
