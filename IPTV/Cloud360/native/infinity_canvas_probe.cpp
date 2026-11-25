// Isolated, bounded GPU test of production canvas. No writes to profiles,
// no launch, no playback, no network. Test only after no-GPU diagnostics.
#include "HomeCanvas.h"
#include "GameScanner.h"
#include "TvCatalog.h"
#ifdef INFINITY_FULL_MENU_PREVIEW
#include "ConsoleHome.h"
#include "XexLaunchCheck.h"
#ifdef INFINITY_XSDK_HTTPS
#include "XsdkDownload.h"
#ifdef INFINITY_STORE_ZIP
#include "StoreZipInstaller.h"
#endif
#include "DiscInstaller.h"
#include "StorageInfo.h"
#include <chrono>
#endif
#include "xsdk/xam.h"
#include <cerrno>
#include <filesystem>
#include "infinity_menu_preview_shaders.h"
#include <ctime>
#else
#include "infinity_canvas_probe_shaders.h"
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
    unsigned width=0,height=0;
    bool Open(){
        if(!NT_SUCCESS(xsdk_gpu_open()))return false;
        gpu=true;
        if(!NT_SUCCESS(xsdk_shader_load(&vs,infinity_canvas_vert.container,infinity_canvas_vert.container_bytes)) ||
           !NT_SUCCESS(xsdk_shader_load(&ps,infinity_canvas_frag.container,infinity_canvas_frag.container_bytes)))return false;
        XsdkVertexElement layout[2]{};
        layout[0].usage=XSDK_USAGE_POSITION;layout[0].format=XSDK_VERTEX_FLOAT3;
        layout[1].usage=XSDK_USAGE_TEXCOORD;layout[1].format=XSDK_VERTEX_FLOAT2;layout[1].offset_bytes=12;
        const uint32_t stride[]={sizeof quad[0]};
        if(!NT_SUCCESS(xsdk_shader_set_vertex_layout(&vs,layout,2,stride)))return false;
        vertices=xsdk_gpu_alloc(sizeof quad);
        if(!vertices)return false;
        for(unsigned i=0;i<sizeof quad/4;++i){uint32_t word;std::memcpy(&word,reinterpret_cast<const char*>(quad)+4*i,4);vertices[i]=word;}
        xsdk_gpu_invalidate(xsdk_gpu_address(vertices),sizeof quad);
        if(!NT_SUCCESS(xsdk_texture_create(&texture,XSDK_FORMAT_A8R8G8B8,HomeCanvas::Width,HomeCanvas::Height,1,true)))return false;
        XsdkSampler cfg{};cfg.mag_filter=cfg.min_filter=XSDK_FILTER_POINT;
        cfg.address_u=cfg.address_v=XSDK_ADDRESS_CLAMP;
        xsdk_texture_sampler_state(&sampler,&cfg);
        uint32_t w=0,h=0;
        if(!NT_SUCCESS(xsdk_present_open(XSDK_DISPLAY_720P,3,1,&w,&h)))return false;
        present=true;width=w;height=h;
        return width==HomeCanvas::Width && height==HomeCanvas::Height;
    }
    bool Show(const HomeCanvas& canvas,bool upload){
        // A single texture is reused only after the prior frame has finished.
        if(upload){
            if(!NT_SUCCESS(xsdk_gpu_wait(xsdk_gpu_fence(),Timeout)))return false;
            xsdk_texture_upload(&texture,0,canvas.Pixels(),HomeCanvas::Width*4);
            xsdk_gpu_invalidate(texture.address,texture.size_bytes);
        }
        uint32_t address=0;
        if(!NT_SUCCESS(xsdk_present_begin(&address,Timeout)))return false;
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
#ifndef INFINITY_FULL_MENU_PREVIEW
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
#ifndef INFINITY_FULL_MENU_PREVIEW
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
    std::ifstream launchMarker((dir+"enable-xex-launch.txt").c_str());
    std::string launchSignature;
    const bool launchEnabled=bool(std::getline(launchMarker,launchSignature)) && launchSignature=="ENABLE-XEX-LAUNCH";
    launchMarker.close();
    std::string pendingLaunch,confirmedLaunch,launchNotice;
    Presenter p;
    if(!p.Open()){xsdk_log_write("FAIL: menu presenter open");p.Close();return 3;}
    const uint16_t keys[]={INPUT_GAMEPAD_DPAD_LEFT,INPUT_GAMEPAD_DPAD_RIGHT,
        INPUT_GAMEPAD_DPAD_UP,INPUT_GAMEPAD_DPAD_DOWN,INPUT_GAMEPAD_A,
        INPUT_GAMEPAD_B,INPUT_GAMEPAD_X,INPUT_GAMEPAD_Y,0x0100,0x0200,
        INPUT_GAMEPAD_START,0x0080};
    uint16_t previous=0;
    bool ok=true,dirty=true,systemExit=false;
    // Longer than the minimal GPU probe, but still bounded.
    for(unsigned frame=0;frame<3600;++frame){
        INPUT_STATE input{};
        uint16_t buttons=XamInputGetState(0,0,&input)==ERROR_SUCCESS?input.gamepad.buttons:0;
        if(input.gamepad.thumb_lx < -16000)buttons|=INPUT_GAMEPAD_DPAD_LEFT;
        if(input.gamepad.thumb_lx > 16000)buttons|=INPUT_GAMEPAD_DPAD_RIGHT;
        if(input.gamepad.thumb_ly > 16000)buttons|=INPUT_GAMEPAD_DPAD_UP;
        if(input.gamepad.thumb_ly < -16000)buttons|=INPUT_GAMEPAD_DPAD_DOWN;
        const uint16_t edge=buttons&~previous;previous=buttons;
        if(edge&INPUT_GAMEPAD_BACK)break;
        if(!pendingLaunch.empty()){
            if(edge&INPUT_GAMEPAD_B){pendingLaunch.clear();launchNotice="Lancamento cancelado.";dirty=true;}
            else if(edge&INPUT_GAMEPAD_A){confirmedLaunch=pendingLaunch;break;}
        }else for(unsigned i=0;i<sizeof keys/sizeof keys[0];++i)if(edge&keys[i]){
            launchNotice.clear();
            const std::string launch=menu.Press(static_cast<ConsoleHome::Key>(i));
            if(!launch.empty()){
                const std::string path=XexLaunchCheck::Path(launch);
                std::error_code ec;
                if(!launchEnabled)launchNotice="XEX bloqueado: habilite o teste em MenuPreview.";
                else if(path.empty()||!std::filesystem::is_regular_file(path,ec)||ec)launchNotice="XEX invalido ou indisponivel.";
                else {
                    std::ifstream file(path.c_str(),std::ios::binary);
                    if(!XexLaunchCheck::Header(file))launchNotice="Cabecalho XEX2 invalido ou incompleto.";
                    else pendingLaunch=path;
                }
            }
            if(!menu.TakeMovie().empty())menu.PlayerMessage("Player ainda indisponivel no XSDK.");
            StoreItem pending;
#ifdef INFINITY_XSDK_HTTPS
            if(menu.TakeDiscSystemExit()){systemExit=true;break;}
            std::string mount;XboxExecutionInfo identity{};
            if(menu.TakeDiscInstallRequest(mount,identity)){
                menu.StopFtp();FileWorkspace workspace(dir);
                bool videoFailed=false;
                auto last=std::chrono::steady_clock::now()-std::chrono::seconds(1);
                const auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};
                    if(XamInputGetState(0,0,&state)==ERROR_SUCCESS &&
                       (state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    const auto now=std::chrono::steady_clock::now();
                    if(done!=total&&now-last<std::chrono::milliseconds(200))return true;
                    last=now;canvas.Background();
                    canvas.Text(48,100,"INFINITY 360 | Instalar DVD",36,WHITE,1180);
                    canvas.Text(48,210,"Destino: "+mount,28,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);
                    if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    canvas.Text(48,560,"B Cancelar | Copia + verificacao de leitura",22,LIME,1180);
                    if(!p.Show(canvas,true)){videoFailed=true;return false;}
                    return true;
                };
                std::string installed,error;
                const bool success=InstallDiscFiles(workspace,dir,"dvd:/",mount,progress,installed,error,&identity);
                menu.DiscInstallResult(success?"DVD instalado e verificado: "+installed:error,success);
                if(videoFailed){ok=false;break;}
            }
            if(menu.TakeStoreInstallRequest(pending)){
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
                    canvas.Text(48,100,"INFINITY Store | Instalar pacote",36,WHITE,1180);
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
                const bool success=StoreInstallPackage(workspace,pending,dir,progress,error);
#else
                const bool success=StoreInstallXex(workspace,pending,dir,progress,error);
#endif
                menu.StoreInstallResult(success?"Pacote instalado. Abra Apps, Homebrew ou Emuladores.":error,success);
                if(videoFailed){ok=false;break;}
            }
#endif
            while(menu.TakeNativeStoreRequest(pending)){
#ifdef INFINITY_XSDK_HTTPS
                menu.StopFtp();
                bool success=false;
                bool videoFailed=false;
                const auto started=std::chrono::steady_clock::now();
                unsigned long long initial=0;bool firstProgress=true;
                auto last=started-std::chrono::seconds(1);
                const auto progress=[&](unsigned long long done,unsigned long long total){
                    INPUT_STATE state{};
                    if(XamInputGetState(0,0,&state)==ERROR_SUCCESS &&
                       (state.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK)))return false;
                    if(firstProgress){initial=done;firstProgress=false;}
                    const auto now=std::chrono::steady_clock::now();
                    if(now-last<std::chrono::milliseconds(200))return true;
                    last=now;
                    canvas.Background();
                    canvas.Text(48,100,"INFINITY Store | HTTPS",36,WHITE,1180);
                    canvas.Text(48,210,pending.name,28,WHITE,1180);
                    canvas.Text(48,300,StorageInfo::FormatBytes(done)+" / "+StorageInfo::FormatBytes(total),24,MUTED,1180);
                    canvas.Box(48,370,1180,22,0xff28413au,5);
                    if(total)canvas.Box(48,370,static_cast<int>(1180.0*done/total),22,LIME,5);
                    const auto elapsed=std::chrono::duration_cast<std::chrono::seconds>(now-started).count();
                    if(elapsed>0)canvas.Text(48,430,StorageInfo::FormatBytes((done-initial)/static_cast<unsigned long long>(elapsed))+"/s (media)",22,MUTED,1180);
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
        if(menu.Tick())dirty=true;
        if(dirty){
            menu.Draw(canvas,"XSDK");
            canvas.Box(895,13,180,45,0xff102923u,5);
            canvas.Text(905,20,"ALPHA TESTE",20,LIME,170);
            if(!launchNotice.empty()){
                canvas.Box(40,590,1200,38,0xff102923u,6);
                canvas.Text(54,593,launchNotice,22,LIME,1160);
            }
            if(!pendingLaunch.empty()){
                canvas.Box(180,220,920,260,LIME,12);
                canvas.Box(184,224,912,252,0xff102923u,10);
                canvas.Text(210,242,"Executar XEX selecionado?",32,WHITE,850);
                canvas.Text(210,310,pendingLaunch,20,MUTED,850);
                canvas.Text(210,392,"A Confirmar | B Cancelar | retorna pela dashboard do console",20,LIME,850);
            }
        }
        if(!p.Show(canvas,dirty)){ok=false;break;}
        dirty=false;
    }
    menu.StopFtp();
    const bool restored=p.Close();
    xsdk_log_write("%s: full menu preview GPU return",ok&&restored?"PASS":"FAIL");
    if(systemExit){
        xsdk_log_write("NXE handoff requested; normal title exit, not automatic installation.");
        return ok&&restored?0:4;
    }
    if(ok&&restored&&!confirmedLaunch.empty()){
        // Recheck after shutting down GPU: removable storage may have changed.
        std::error_code ec;
        if(!std::filesystem::is_regular_file(confirmedLaunch,ec)||ec)return 5;
        {std::ifstream file(confirmedLaunch.c_str(),std::ios::binary);
         if(!XexLaunchCheck::Header(file))return 5;}
        xsdk_log_write("XEX launch requested: %s",confirmedLaunch.c_str());
        (void)xsdk_launch(confirmedLaunch.c_str(),nullptr,0);
        xsdk_log_write("FAIL: xsdk_launch returned, errno=%d",errno);
        return 6;
    }
    return ok&&restored?0:4;
}
#endif
