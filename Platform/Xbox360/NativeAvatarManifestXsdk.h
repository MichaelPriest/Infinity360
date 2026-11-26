#pragma once
#include "AvatarManifest.h"
#include <xsdk/kernel.h>
#include <fstream>
namespace InfinitySocial {
inline AvatarManifestSummary ReadNativeAvatarManifest(const Header& expected,const std::string& dir){
    if(!CanRequestNative(NativeAction::GamerCard,expected)||XamIsUIActive())return {};
    std::ifstream flag(dir+"enable-native-avatar-manifest.txt");
    std::string line;
    if(!std::getline(flag,line)||(line!="ENABLE-NATIVE-AVATAR-MANIFEST"&&line!="ENABLE-NATIVE-AVATAR-MANIFEST\r"))return {};
    HANDLE module=0;void* address=nullptr;
    if(XexGetModuleHandle("xam.xex",&module)!=0||!module||
       XexGetProcedureAddress(module,0x5deu,&address)!=0||!address)return {};
    const auto user=static_cast<std::uint32_t>(expected.controller);
    const auto state=expected.state==SignIn::Online?2u:1u;
    const auto verify=[&](){USER_SIGNIN_INFO info{};return XamUserGetSigninInfo(user,0,&info)==0&&
        info.xuid==expected.identity&&info.signin_state==state&&XamUserGetSigninState(user)==state;};
    if(!verify())return {};
    // Opaque bounded buffer, synchronous read only; no XAvatarInitialize,
    // GetAssets or animation calls. Result reduced to component presence.
    alignas(128) std::array<unsigned char,1024> bytes{};
    using Read=std::uint32_t(*)(std::uint32_t,void*,void*);
    const auto rc=reinterpret_cast<Read>(address)(user,bytes.data(),nullptr);
    if(rc!=0||!verify())return {};
    return InspectAvatarManifest(bytes.data(),AvatarManifestBytes);
}
}
