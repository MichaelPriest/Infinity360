// Xbox 360 XDK presenter. The menu, catalogue and drawing code also run on PC.
#include <xtl.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <cstdio>
#include <cstring>
#include "ConsoleHome.h"

namespace {
template<class T> void drop(T*& object) { if(object) object->Release(); object=0; }
struct Vertex { float x,y,u,v; };
const Vertex screen[]={{-1,1,0,0},{1,1,1,0},{-1,-1,0,1},{1,-1,1,1}};

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
        device_->SetRenderState(D3DRS_ZENABLE,FALSE);
        device_->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);
        device_->SetSamplerState(0,D3DSAMP_ADDRESSU,D3DTADDRESS_CLAMP);
        device_->SetSamplerState(0,D3DSAMP_ADDRESSV,D3DTADDRESS_CLAMP);
        device_->SetSamplerState(0,D3DSAMP_MAGFILTER,D3DTEXF_POINT);
        device_->SetSamplerState(0,D3DSAMP_MINFILTER,D3DTEXF_POINT);
        return true;
    }
    bool Show(const HomeCanvas& canvas) {
        D3DLOCKED_RECT lock;
        device_->SetTexture(0,0);
        if(FAILED(texture_->LockRect(0,&lock,0,0))) return false;
        for(int y=0;y<HomeCanvas::Height;++y)
            std::memcpy(static_cast<char*>(lock.pBits)+y*lock.Pitch,canvas.Pixels()+y*HomeCanvas::Width,
                        HomeCanvas::Width*sizeof(unsigned));
        texture_->UnlockRect(0);
        device_->SetTexture(0,texture_); device_->SetVertexDeclaration(decl_);
        device_->SetVertexShader(vertex_); device_->SetPixelShader(pixel_);
        device_->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,screen,sizeof(Vertex));
        device_->Present(0,0,0,0);
        return true;
    }
};

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
        XINPUT_GAMEPAD_DPAD_DOWN,XINPUT_GAMEPAD_A,XINPUT_GAMEPAD_B,XINPUT_GAMEPAD_X,XINPUT_GAMEPAD_Y};
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
