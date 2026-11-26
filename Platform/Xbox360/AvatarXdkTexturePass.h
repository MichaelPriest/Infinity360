#pragma once
#include "AvatarXdkRenderer.h"
#if defined(INFINITY_ENABLE_XDK_AVATAR)
namespace InfinityXdk {
// Separate D3D9 pass, scheduled BEFORE the dashboard draws its background.
// The host must own this EDRAM region exclusively, finish previous GPU work,
// restore/rebind its render states and redraw its own targets afterwards.
// Restoring surface pointers does not restore overwritten EDRAM contents.
class AvatarTexturePass {
    IDirect3DDevice9* device_;
    IDirect3DSurface9* color_;
    IDirect3DSurface9* depth_;
    IDirect3DTexture9* texture_;
    bool valid_;
    AvatarTexturePass(const AvatarTexturePass&);
    AvatarTexturePass& operator=(const AvatarTexturePass&);
public:
    enum { Size=512, TileHeight=256 };
    AvatarTexturePass():device_(NULL),color_(NULL),depth_(NULL),texture_(NULL),valid_(false){}
    ~AvatarTexturePass(){Close();}
    void Close(){
        valid_=false;
        if(texture_){texture_->Release();texture_=NULL;}
        if(depth_){depth_->Release();depth_=NULL;}
        if(color_){color_->Release();color_=NULL;}
        if(device_){device_->Release();device_=NULL;}
    }
    HRESULT Open(IDirect3DDevice9* device){
        if(device_||!device)return E_INVALIDARG;
        device_=device;device_->AddRef();
        D3DSURFACE_PARAMETERS p={0};
        HRESULT hr=device_->CreateRenderTarget(Size,TileHeight,D3DFMT_A8R8G8B8,
            D3DMULTISAMPLE_4_SAMPLES,0,0,&color_,&p);
        if(FAILED(hr)||!color_){Close();return FAILED(hr)?hr:E_FAIL;}
        p.Base=color_->Size/GPU_EDRAM_TILE_SIZE;p.HierarchicalZBase=0;
        hr=device_->CreateDepthStencilSurface(Size,TileHeight,D3DFMT_D24S8,
            D3DMULTISAMPLE_4_SAMPLES,0,0,&depth_,&p);
        if(FAILED(hr)||!depth_){Close();return FAILED(hr)?hr:E_FAIL;}
        hr=device_->CreateTexture(Size,Size,1,0,D3DFMT_A8R8G8B8,D3DPOOL_DEFAULT,&texture_,NULL);
        if(FAILED(hr)||!texture_){Close();return FAILED(hr)?hr:E_FAIL;}
        return S_OK;
    }
    // Host calls between frames, not inside another predicated tiling pass.
    // Caller supplies view/projection to frame the original avatar.
    bool Render(AvatarRenderer& renderer,const XMMATRIX& world,
        const XMMATRIX& view,const XMMATRIX& projection){
        valid_=false;
        if(!device_||!texture_||renderer.Device()!=device_||!renderer.Ready())return false;
        IDirect3DSurface9* previousColor=NULL;
        IDirect3DSurface9* previousDepth=NULL;
        device_->GetRenderTarget(0,&previousColor);
        device_->GetDepthStencilSurface(&previousDepth);
        if(!previousColor){if(previousDepth)previousDepth->Release();return false;}
        device_->SetRenderTarget(0,color_);
        device_->SetDepthStencilSurface(depth_);
        device_->SetRenderState(D3DRS_COLORWRITEENABLE,D3DCOLORWRITEENABLE_ALL);
        const D3DVECTOR4 clear={0,0,0,0};
        const D3DRECT tiles[2]={{0,0,Size,TileHeight},{0,TileHeight,Size,Size}};
        device_->BeginTiling(0,2,tiles,&clear,1,0);
        const bool submitted=renderer.Draw(world,view,projection);
        device_->EndTiling(0,NULL,texture_,NULL,1,0,NULL);
        device_->SetRenderTarget(0,previousColor);
        device_->SetDepthStencilSurface(previousDepth);
        previousColor->Release();if(previousDepth)previousDepth->Release();
        valid_=submitted&&renderer.Ready();
        return valid_;
    }
    // Borrowed GPU texture. Host must recheck profile and order sampling
    // after resolve; never cache the texture across identity changes.
    IDirect3DTexture9* Texture(const AvatarRenderer& renderer)const{
        return valid_&&renderer.Device()==device_&&renderer.Ready()?texture_:NULL;
    }
    void Invalidate(){valid_=false;}
};
}
#endif
