// Xbox 360 XDK presenter. The menu, catalogue and drawing code also run on PC.
#include <xtl.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <xaudio2.h>
#include <xmedia2.h>
#include <cstdio>
#include <cstring>
#include "ConsoleHome.h"
#include "XboxHttpStream.h"

namespace {
template<class T> void drop(T*& object) { if(object) object->Release(); object=0; }
struct Vertex { float x,y,u,v; };
const Vertex screen[]={{-1,1,0,0},{1,1,1,0},{-1,-1,0,1},{1,-1,1,1}};

WORD buttons();
bool stopMovie() { return (buttons() & (XINPUT_GAMEPAD_B | XINPUT_GAMEPAD_BACK)) != 0; }
class LocalMovie {
    IXAudio2* audio_;
    IXAudio2MasteringVoice* master_;
    IXMedia2XmvPlayer* player_;
public:
    LocalMovie() : audio_(0),master_(0),player_(0) {}
    ~LocalMovie() {
        if(player_) player_->Stop(XMEDIA_STOP_IMMEDIATE);
        drop(player_);
        if(master_) master_->DestroyVoice();
        drop(audio_);
    }
    std::string Play(IDirect3DDevice9* device,const std::string& path) {
        if(FAILED(XAudio2Create(&audio_,0)) || FAILED(audio_->CreateMasteringVoice(&master_)))
            return "Não foi possível iniciar a saída de áudio.";
        XMEDIA_XMV_CREATE_PARAMETERS source;
        ZeroMemory(&source,sizeof source);
        source.createType=XMEDIA_CREATE_FROM_FILE;
        source.createFromFile.szFileName=path.c_str();
        source.dwAudioStreamId=XMEDIA_STREAM_ID_USE_DEFAULT;
        source.dwVideoStreamId=XMEDIA_STREAM_ID_USE_DEFAULT;
        if(FAILED(XMedia2CreateXmvPlayer(device,audio_,&source,&player_)))
            return "Não foi possível abrir o WMV. Confira o arquivo e o codec.";
        XMEDIA_VIDEO_SCREEN area;
        ZeroMemory(&area,sizeof area);
        for(unsigned i=0;i<4;++i) {
            const float u=(i&1)?1.0f:0.0f,v=(i&2)?1.0f:0.0f;
            area.aVertices[i].fX=u*HomeCanvas::Width;area.aVertices[i].fY=v*HomeCanvas::Height;
            area.aVertices[i].fTu=u;area.aVertices[i].fTv=v;
        }
        player_->SetVideoScreen(&area);
        while(!(buttons()&(XINPUT_GAMEPAD_B|XINPUT_GAMEPAD_BACK))) {
            const HRESULT result=player_->RenderNextFrame(0,0);
            if(result==(HRESULT)XMEDIA_W_EOF) return "Reprodução concluída.";
            if(FAILED(result)) return "A reprodução foi interrompida pelo decodificador.";
            if(result==S_OK) device->Present(0,0,0,0);
            else Sleep(4);
        }
        return "";
    }
};
class Presenter {
    IDirect3D9* d3d_;
    IDirect3DDevice9* device_;
    IDirect3DTexture9* texture_;
    IDirect3DVertexShader9* vertex_;
    IDirect3DPixelShader9* pixel_;
    IDirect3DVertexDeclaration9* decl_;
    bool Shader(const char* source,const char* profile,bool vertex) {
        ID3DXBuffer *code=0,*errors=0;
        HRESULT hr=D3DXCompileShader(source,static_cast<UINT>(std::strlen(source)),0,0,"main",
                                     profile,0,&code,&errors,0);
        if(errors) OutputDebugStringA(static_cast<const char*>(errors->GetBufferPointer()));
        drop(errors);
        if(SUCCEEDED(hr) && code) {
            const DWORD* words=static_cast<const DWORD*>(code->GetBufferPointer());
            hr=vertex ? device_->CreateVertexShader(words,&vertex_) : device_->CreatePixelShader(words,&pixel_);
        }
        drop(code);
        return SUCCEEDED(hr);
    }
public:
    Presenter() : d3d_(0),device_(0),texture_(0),vertex_(0),pixel_(0),decl_(0) {}
    ~Presenter() {
        if(device_) device_->BlockUntilIdle();
        drop(decl_); drop(pixel_); drop(vertex_); drop(texture_); drop(device_); drop(d3d_);
    }
    bool Open() {
        d3d_=Direct3DCreate9(D3D_SDK_VERSION);
        if(!d3d_) return false;
        D3DPRESENT_PARAMETERS p;
        ZeroMemory(&p,sizeof p);
        p.BackBufferWidth=HomeCanvas::Width; p.BackBufferHeight=HomeCanvas::Height;
        p.BackBufferFormat=D3DFMT_A8R8G8B8; p.BackBufferCount=1;
        p.SwapEffect=D3DSWAPEFFECT_DISCARD; p.PresentationInterval=D3DPRESENT_INTERVAL_ONE;
        if(FAILED(d3d_->CreateDevice(0,D3DDEVTYPE_HAL,0,D3DCREATE_HARDWARE_VERTEXPROCESSING,&p,&device_))) return false;
        const D3DVERTEXELEMENT9 elements[]={
            {0,0,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
            {0,8,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0}, D3DDECL_END()
        };
        if(FAILED(device_->CreateVertexDeclaration(elements,&decl_)) ||
           FAILED(device_->CreateTexture(HomeCanvas::Width,HomeCanvas::Height,1,0,D3DFMT_LIN_A8R8G8B8,
                                         D3DPOOL_DEFAULT,&texture_,0))) return false;
        if(!Shader("void main(float2 p:POSITION,float2 t:TEXCOORD0,out float4 o:POSITION,out float2 uv:TEXCOORD0)"
                   "{o=float4(p,0,1);uv=t;}","vs_3_0",true) ||
           !Shader("sampler2D frame:register(s0);float4 main(float2 uv:TEXCOORD0):COLOR0"
                   "{return tex2D(frame,uv);}","ps_3_0",false)) return false;
        SetMenuState();
        return true;
    }
    void SetMenuState() {
        device_->SetRenderState(D3DRS_VIEWPORTENABLE,TRUE);
        device_->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
        device_->SetRenderState(D3DRS_ZENABLE,FALSE);
        device_->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
        device_->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
        device_->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
        device_->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
        device_->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);
    }
    std::string Play(const std::string& path);
    void DrawTexture(IDirect3DTexture9* texture, const Vertex* vertices) {
        device_->SetTexture(0,texture); device_->SetVertexDeclaration(decl_);
        device_->SetVertexShader(vertex_); device_->SetPixelShader(pixel_);
        device_->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(Vertex));
        device_->Present(0,0,0,0);
    }
    bool Show(const HomeCanvas& canvas) {
        D3DLOCKED_RECT lock;
        device_->SetTexture(0,0);
        if(FAILED(texture_->LockRect(0,&lock,0,0))) return false;
        for(int y=0;y<HomeCanvas::Height;++y)
            std::memcpy(static_cast<char*>(lock.pBits)+y*lock.Pitch,canvas.Pixels()+y*HomeCanvas::Width,
                        HomeCanvas::Width*sizeof(unsigned));
        texture_->UnlockRect(0);
        DrawTexture(texture_,screen);
        return true;
    }
};

class NetworkMovie : public MpegOutput {
    enum { Slots=12, Block=PLM_AUDIO_SAMPLES_PER_FRAME*2 };
    IDirect3DDevice9* device_;
    Presenter& display_;
    IDirect3DTexture9* picture_;
    IXAudio2* audio_;
    IXAudio2MasteringVoice* master_;
    IXAudio2SourceVoice* voice_;
    std::vector<short> pcm_;
    unsigned slot_,width_,height_;
    double aspect_;
    bool WaitForAudio(unsigned limit) {
        const DWORD start=GetTickCount();
        while(voice_) {
            XAUDIO2_VOICE_STATE state;
            voice_->GetState(&state);
            if(state.BuffersQueued<=limit) break;
            if(stopMovie() || GetTickCount()-start>2000) return false;
            Sleep(2);
        }
        return true;
    }
public:
    NetworkMovie(IDirect3DDevice9* device,Presenter& display) : device_(device),display_(display),
        picture_(0),audio_(0),master_(0),voice_(0),pcm_(Slots*Block),slot_(0),width_(0),height_(0),aspect_(1) {}
    ~NetworkMovie() {
        if(voice_) { voice_->Stop();voice_->DestroyVoice(); }
        if(master_) master_->DestroyVoice();
        drop(audio_);
        device_->SetTexture(0,0);
        drop(picture_);
    }
    bool Video(plm_frame_t& frame) {
        if(!picture_) {
            width_=frame.width;height_=frame.height;
            if(FAILED(device_->CreateTexture(width_,height_,1,0,D3DFMT_LIN_X8R8G8B8,
                                             D3DPOOL_DEFAULT,&picture_,0))) return false;
        }
        if(frame.width!=width_ || frame.height!=height_) return false;
        device_->SetTexture(0,0);
        D3DLOCKED_RECT lock;
        if(FAILED(picture_->LockRect(0,&lock,0,0))) return false;
        // Xenon stores packed ARGB words in big-endian byte order.
        plm_frame_to_argb(&frame,static_cast<unsigned char*>(lock.pBits),lock.Pitch);
        picture_->UnlockRect(0);
        const double ratio=width_*aspect_*HomeCanvas::Height/(height_*HomeCanvas::Width);
        Vertex quad[4];
        for(unsigned i=0;i<4;++i) {
            quad[i]=screen[i];
            if(ratio<1) quad[i].x*=static_cast<float>(ratio);
            else quad[i].y/=static_cast<float>(ratio);
        }
        device_->Clear(0,0,D3DCLEAR_TARGET,0xff000000u,1,0);
        display_.DrawTexture(picture_,quad);
        return true;
    }
    bool Audio(const plm_samples_t& samples) {
        if(!voice_ || samples.count!=PLM_AUDIO_SAMPLES_PER_FRAME || !WaitForAudio(Slots-1)) return false;
        short* dst=&pcm_[slot_*Block];
        mpegStereoPcm16(samples,dst);
        XAUDIO2_BUFFER buffer;
        ZeroMemory(&buffer,sizeof buffer);
        buffer.pAudioData=reinterpret_cast<const BYTE*>(dst);buffer.AudioBytes=Block*sizeof(short);
        if(FAILED(voice_->SubmitSourceBuffer(&buffer))) return false;
        slot_=(slot_+1)%Slots;
        return true;
    }
    std::string Play(const MediaUrl& url) {
        XboxHttpStream stream(stopMovie);
        if(!stream.Open(url)) return stream.Cancelled() ? "" : stream.Error();
        MpegPlayback movie(stream,*this);
        if(!movie.Open()) {
            if(stream.Cancelled()) return "";
            return stream.Error().empty() ? movie.Error() : stream.Error();
        }
        aspect_=movie.PixelAspect();if(!(aspect_>0)) aspect_=1;
        if(movie.SampleRate()>0) {
            WAVEFORMATEX format;
            ZeroMemory(&format,sizeof format);
            format.wFormatTag=WAVE_FORMAT_PCM;format.nChannels=2;format.nSamplesPerSec=movie.SampleRate();
            format.wBitsPerSample=16;format.nBlockAlign=4;format.nAvgBytesPerSec=format.nSamplesPerSec*4;
            if(FAILED(XAudio2Create(&audio_,0)) || FAILED(audio_->CreateMasteringVoice(&master_)) ||
               FAILED(audio_->CreateSourceVoice(&voice_,&format)) || FAILED(voice_->Start()))
                return "Não foi possível iniciar a saída de áudio.";
        }
        device_->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);
        device_->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);
        DWORD last=GetTickCount();
        while(!stopMovie()) {
            const DWORD now=GetTickCount(),delta=now-last;
            if(delta) {
                last=now;
                if(!movie.Advance((delta>80 ? 80 : delta)*0.001)) {
                    if(stream.Cancelled() || stopMovie()) return "";
                    if(!stream.Error().empty()) return stream.Error();
                    if(!movie.Error().empty()) return movie.Error();
                    if(!WaitForAudio(0)) return stopMovie() ? "" : "A saída de áudio parou de responder.";
                    return "Transmissão encerrada.";
                }
            }
            Sleep(2);
        }
        return "";
    }
};
std::string Presenter::Play(const std::string& path) {
    std::string status;
    MediaUrl url;
    if(url.Parse(path)) { NetworkMovie movie(device_,*this);status=movie.Play(url); }
    else { LocalMovie movie;status=movie.Play(device_,path); }
    SetMenuState();
    return status;
}

WORD buttons() {
    XINPUT_STATE s;
    if(XInputGetState(0,&s)!=ERROR_SUCCESS) return 0;
    WORD b=s.Gamepad.wButtons;
    const int threshold=16000;
    if(s.Gamepad.sThumbLX < -threshold) b|=XINPUT_GAMEPAD_DPAD_LEFT;
    if(s.Gamepad.sThumbLX >  threshold) b|=XINPUT_GAMEPAD_DPAD_RIGHT;
    if(s.Gamepad.sThumbLY >  threshold) b|=XINPUT_GAMEPAD_DPAD_UP;
    if(s.Gamepad.sThumbLY < -threshold) b|=XINPUT_GAMEPAD_DPAD_DOWN;
    return b;
}
}

int main() {
    HomeCanvas canvas;
    if(!canvas.LoadArt("game:\\home.tga")) { OutputDebugStringA("AuroraX: home.tga ausente ou invalido.\n"); return 1; }
    Presenter video;
    if(!video.Open()) return 2;
    ConsoleHome menu("game:\\",GameScanner::DefaultRoots()); menu.Load();
    const WORD keys[]={XINPUT_GAMEPAD_DPAD_LEFT,XINPUT_GAMEPAD_DPAD_RIGHT,XINPUT_GAMEPAD_DPAD_UP,
        XINPUT_GAMEPAD_DPAD_DOWN,XINPUT_GAMEPAD_A,XINPUT_GAMEPAD_B,XINPUT_GAMEPAD_X,XINPUT_GAMEPAD_Y,
        XINPUT_GAMEPAD_LEFT_SHOULDER,XINPUT_GAMEPAD_RIGHT_SHOULDER,XINPUT_GAMEPAD_START,XINPUT_GAMEPAD_RIGHT_THUMB};
    WORD previous=buttons();
    DWORD heldSince=GetTickCount(),lastRepeat=heldSince;
    int minute=-1;
    for(;;) {
        WORD b=buttons(), edge=b&~previous;
        const WORD directions=b&15;
        const DWORD now=GetTickCount();
        if(directions!=(previous&15)) heldSince=lastRepeat=now;
        else if(directions && now-heldSince>380 && now-lastRepeat>110) {edge|=directions;lastRepeat=now;}
        previous=b;
        if(edge&XINPUT_GAMEPAD_BACK) break;
        for(size_t i=0;i<sizeof(keys)/sizeof(keys[0]);++i) if(edge&keys[i]) {
            std::string path=menu.Press(static_cast<ConsoleHome::Key>(i));
            if(!path.empty()) { XLaunchNewImage(path.c_str(),0); menu.LaunchFailed(); }
            const std::string movie=menu.TakeMovie();
            if(!movie.empty()) {
                menu.PlayerMessage("Abrindo vídeo...  B para voltar.");
                menu.Draw(canvas,"");video.Show(canvas);
                menu.PlayerMessage(video.Play(movie));
                previous=buttons();heldSince=lastRepeat=GetTickCount();
                minute=-1;
            }
            break;
        }
        SYSTEMTIME time; GetLocalTime(&time);
        if(edge || minute!=time.wMinute) {
            char clock[12]; std::sprintf(clock,"%02u:%02u",unsigned(time.wHour),unsigned(time.wMinute));
            minute=time.wMinute; menu.Draw(canvas,clock);
            if(!video.Show(canvas)) return 3;
        }
        Sleep(16);
    }
    return 0;
}
