#pragma once
// Optional official XDK path. The current XSDK/Xenos presenter must NOT
// include or link this header. Requires a caller-owned Xbox D3D9 device,
// licensed XDK headers/libraries and exclusive XAvatar library ownership.
#if defined(INFINITY_ENABLE_XDK_AVATAR)
#include <xtl.h>
#include <xavatar.h>
#include <xnamath.h>
namespace InfinityXdk {
class AvatarRenderer {
    IDirect3DDevice9* device_;
    LPXAVATARRENDERER renderer_;
    LPXAVATARANIMATION wave_,clap_;
    XAVATAR_METADATA metadata_;
    DWORD user_;
    XUID owner_;
    XUSER_SIGNIN_STATE state_;
    bool initialized_;
    AvatarRenderer(const AvatarRenderer&);
    AvatarRenderer& operator=(const AvatarRenderer&);
    bool SameAccount()const{
        if(!owner_||user_>=4||XUserGetSigninState(user_)!=state_)return false;
        XUID current=0;
        return XUserGetXUID(user_,&current)==ERROR_SUCCESS&&current==owner_;
    }
    void ReleaseProfile(){
        if(renderer_){renderer_->Release();renderer_=NULL;}
        ZeroMemory(&metadata_,sizeof(metadata_));
        owner_=0;user_=4;
    }
public:
    AvatarRenderer():device_(NULL),renderer_(NULL),wave_(NULL),clap_(NULL),user_(4),
        owner_(0),state_(eXUserSigninState_NotSignedIn),initialized_(false){
        ZeroMemory(&metadata_,sizeof(metadata_));
    }
    // Host owns the D3D device and must finish GPU work before destruction,
    // Close, or LoadLocal. Never run alongside a direct-Xenos presenter.
    ~AvatarRenderer(){Close();}
    HRESULT Open(IDirect3DDevice9* device){
        if(initialized_||!device)return E_INVALIDARG;
        HRESULT hr=XAvatarInitialize(XAVATAR_COORDINATE_SYSTEM_RIGHT_HANDED,
            XAVATAR_INITIALIZE_FLAGS_ENABLERENDERER,4,2,device);
        if(FAILED(hr))return hr;
        device_=device;device_->AddRef();initialized_=true;
        // Missing optional animations do not prevent a profile from rendering.
        if(FAILED(XAvatarLoadAnimation(&XAVATAR_ANIMATION_GENERIC_WAVE,0,&wave_))){
            if(wave_)wave_->Release();wave_=NULL;
        }
        if(FAILED(XAvatarLoadAnimation(&XAVATAR_ANIMATION_GENERIC_CLAP,0,&clap_))){
            if(clap_)clap_->Release();clap_=NULL;
        }
        return S_OK;
    }
    void Close(){
        ReleaseProfile();
        if(wave_){wave_->Release();wave_=NULL;}
        if(clap_){clap_->Release();clap_=NULL;}
        if(initialized_){XAvatarShutdown();initialized_=false;}
        if(device_){device_->Release();device_=NULL;}
    }
    // The caller supplies the selected non-guest account, not an arbitrary
    // remote XUID. Never fall back to a random avatar.
    HRESULT LoadLocal(DWORD user,XUID expected,XUSER_SIGNIN_STATE expectedState){
        if(!initialized_||user>=4||!expected||
            (expectedState!=eXUserSigninState_SignedInLocally&&expectedState!=eXUserSigninState_SignedInToLive))return E_INVALIDARG;
        ReleaseProfile();user_=user;owner_=expected;state_=expectedState;
        if(!SameAccount()){ReleaseProfile();return E_FAIL;}
        if(XAvatarGetMetadataLocalUser(user_,&metadata_,NULL)!=ERROR_SUCCESS||!SameAccount()){
            ReleaseProfile();return E_FAIL;
        }
        const HRESULT hr=XAvatarCreateRenderer(&metadata_,XAVATAR_COMPONENT_MASK_ALL,
            XAVATAR_SHADOW_SIZE_SMALL,XAVATAR_MIPMAP_RECOMMENDED_BUFFER_SIZE,0,&renderer_);
        if(FAILED(hr)||!renderer_||!SameAccount()){ReleaseProfile();return FAILED(hr)?hr:E_FAIL;}
        renderer_->SetLevelOfDetail(XAVATAR_RENDER_LOD_LOW);
        return S_OK; // renderer may still be loading; Ready() verifies status.
    }
    bool Ready()const{return renderer_&&SameAccount()&&renderer_->GetStatus()==S_OK;}
    IDirect3DDevice9* Device()const{return device_;}
    HRESULT Update(float dt){
        if(!renderer_||!SameAccount())return E_FAIL;
        if(!(dt>=0.0f&&dt<=0.25f))return E_INVALIDARG;
        return renderer_->Update(dt);
    }
    HRESULT Gesture(bool clap){
        if(!Ready())return E_PENDING;
        LPXAVATARANIMATION animation=clap?clap_:wave_;
        if(!animation)return E_FAIL;
        return renderer_->PlayAnimations(1,&animation,XAVATAR_PLAYANIMATIONS_FLAGS_DEFAULT);
    }
    // Render into the host's current D3D9 pass. The host controls EDRAM
    // target/depth/tiling, resolve, viewport and restores its own state.
    // No competing device creation, CPU readback, or presentation here.
    bool Draw(const XMMATRIX& world,const XMMATRIX& view,const XMMATRIX& projection){
        if(!Ready())return false;
        renderer_->Render(device_,world,view,projection);
        return true; // submitted, not a guarantee of a completed frame.
    }
};
}
#endif
