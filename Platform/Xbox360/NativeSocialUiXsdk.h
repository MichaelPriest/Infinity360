#pragma once
// Experimental native UI delegation; no direct Xbox Live network operations.
#include "SocialProfile.h"
#include <xsdk/kernel.h>
#include <cstdint>
#include <fstream>

namespace InfinitySocial {
inline std::string OpenNativeSocialUI(NativeAction action,const Header& expected,const std::string& dir,std::uint32_t title=0) {
    if(!CanRequestNative(action,expected,title))return "Perfil nao elegivel para esta interface.";
    std::ifstream flag(dir+"enable-native-social-ui.txt");
    std::string line;
    if(!std::getline(flag,line)|| (line!="ENABLE-NATIVE-SOCIAL-UI"&&line!="ENABLE-NATIVE-SOCIAL-UI\r"))
        return "Interface experimental: habilite enable-native-social-ui.txt.";
    if(XamIsUIActive())return "Outra interface Xbox esta aberta.";
    HANDLE module=0;
    void* address=nullptr;
    const std::uint32_t ordinal=action==NativeAction::Achievements?0x2c5u:
        action==NativeAction::GamerCard?0x2c4u:0x2bfu;
    if(XexGetModuleHandle("xam.xex",&module)!=0||!module||
       XexGetProcedureAddress(module,ordinal,&address)!=0||!address)
        return "Interface nao exportada por este sistema.";
    const auto user=static_cast<std::uint32_t>(expected.controller);
    USER_SIGNIN_INFO first{},second{};
    const auto state=expected.state==SignIn::Online?2u:1u;
    if(XamUserGetSigninInfo(user,0,&first)!=0||first.xuid==0||first.signin_state!=state||
       XamUserGetSigninInfo(user,0,&second)!=0||first.xuid!=second.xuid||
       second.signin_state!=state||XamUserGetSigninState(user)!=state)
        return "O perfil mudou; selecione novamente.";
    std::size_t length=0;
    while(length<sizeof(second.name)&&second.name[length]!='\0')++length;
    if(std::string(second.name,length)!=expected.displayName||XamIsUIActive())
        return "O perfil ou a interface mudou; tente novamente.";
    using ShowUI=std::uint32_t(*)(std::uint32_t);
    using ShowAchievements=std::uint32_t(*)(std::uint32_t,std::uint32_t);
    const auto result=action==NativeAction::Achievements?
        reinterpret_cast<ShowAchievements>(address)(user,title):reinterpret_cast<ShowUI>(address)(user);
    return result==0?"Interface Xbox solicitada; acesso depende do sistema.":
                     "O sistema recusou a interface social.";
}
}
