#pragma once
#include "AvatarXdkTexturePass.h"
#if defined(INFINITY_ENABLE_XDK_AVATAR)
namespace InfinityXdk {
// Optional D3D9 host adapter. Host must ensure exclusive EDRAM and finish
// GPU work before Select (if changing account), Clear, or Close.
class AvatarSession {
    AvatarRenderer renderer_;
    AvatarTexturePass pass_;
    bool opened_=false;
    AvatarSession(const AvatarSession&);
    AvatarSession& operator=(const AvatarSession&);
public:
    AvatarSession(){}
    ~AvatarSession(){Close();}
    HRESULT Open(IDirect3DDevice9* device){
        if(opened_||!device)return E_INVALIDARG;
        HRESULT hr=renderer_.Open(device);
        if(FAILED(hr))return hr;
        hr=pass_.Open(device);
        if(FAILED(hr)){renderer_.Close();return hr;}
        opened_=true;return S_OK;
    }
    void Clear(){pass_.Invalidate();renderer_.ClearLocal();}
    void Close(){pass_.Close();renderer_.Close();opened_=false;}
    HRESULT Select(DWORD user,XUID owner,XUSER_SIGNIN_STATE state){
        if(!opened_)return E_FAIL;
        if(renderer_.Matches(user,owner,state))return S_OK;
        Clear();
        return renderer_.LoadLocal(user,owner,state);
    }
    bool Tick(float dt,const XMMATRIX& world,const XMMATRIX& view,const XMMATRIX& projection){
        pass_.Invalidate();
        if(!opened_||!(dt>=0.f&&dt<=0.25f)||FAILED(renderer_.Update(dt)))return false;
        return renderer_.Ready()&&pass_.Render(renderer_,world,view,projection);
    }
    HRESULT Gesture(bool clap){return opened_?renderer_.Gesture(clap):E_FAIL;}
    IDirect3DTexture9* Texture(DWORD user,XUID owner,XUSER_SIGNIN_STATE state)const{
        return opened_&&renderer_.Matches(user,owner,state)?pass_.Texture(renderer_):NULL;
    }
};
}
#endif
