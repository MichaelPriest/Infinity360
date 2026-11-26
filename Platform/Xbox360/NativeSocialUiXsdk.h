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
    if(action==NativeAction::AvatarEditor){
        std::ifstream avatarFlag(dir+"enable-native-avatar-editor.txt");
        std::string avatarLine;
        if(!std::getline(avatarFlag,avatarLine)||(avatarLine!="ENABLE-NATIVE-AVATAR-EDITOR"&&avatarLine!="ENABLE-NATIVE-AVATAR-EDITOR\r"))
            return "Editor experimental: habilite enable-native-avatar-editor.txt.";
    }
    if(XamIsUIActive())return "Outra interface Xbox esta aberta.";
    HANDLE module=0;
    void* address=nullptr;
    const std::uint32_t ordinal=NativeSocialOrdinal(action);
    if(!ordinal)return "Interface social desconhecida.";
    if(XexGetModuleHandle("xam.xex",&module)!=0||!module||
       XexGetProcedureAddress(module,ordinal,&address)!=0||!address)
        return "Interface nao exportada por este sistema.";
    const auto user=static_cast<std::uint32_t>(expected.controller);
    USER_SIGNIN_INFO first{},second{};
    const auto state=expected.state==SignIn::Online?2u:1u;
    if(XamUserGetSigninInfo(user,0,&first)!=0||first.xuid==0||first.xuid!=expected.identity||first.signin_state!=state||
       XamUserGetSigninInfo(user,0,&second)!=0||first.xuid!=second.xuid||
       second.signin_state!=state||XamUserGetSigninState(user)!=state)
        return "O perfil mudou; selecione novamente.";
    std::size_t length=0;
    while(length<sizeof(second.name)&&second.name[length]!='\0')++length;
    if(std::string(second.name,length)!=expected.displayName||XamIsUIActive())
        return "O perfil ou a interface mudou; tente novamente.";
    using ShowUI=std::uint32_t(*)(std::uint32_t);
    using ShowAchievements=std::uint32_t(*)(std::uint32_t,std::uint32_t);
    // This API launches the system editor rather than an overlay, and may
    // terminate the dashboard. Three arguments are required by the reference ABI.
    if(action==NativeAction::AvatarEditor){
        using LaunchEditor=std::int32_t(*)(std::uint32_t,std::uint32_t,const std::uint16_t*);
        const auto status=reinterpret_cast<LaunchEditor>(address)(user,0,nullptr);
        return status==0?"Editor Xbox solicitado; retorno depende do sistema.":"Sistema recusou o editor de avatar.";
    }
    const auto result=action==NativeAction::Achievements?
        reinterpret_cast<ShowAchievements>(address)(user,title):reinterpret_cast<ShowUI>(address)(user);
    return result==0?"Interface Xbox solicitada; acesso depende do sistema.":
                     "O sistema recusou a interface social.";
}
}
