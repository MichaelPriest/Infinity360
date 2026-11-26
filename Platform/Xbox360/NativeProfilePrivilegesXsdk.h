#pragma once
#include "ProfilePrivileges.h"
#include <xsdk/kernel.h>
#include <fstream>
namespace InfinitySocial {
inline std::string ReadNativePrivileges(const Header& expected,const std::string& dir,
        ProfilePrivileges& result,std::uint64_t now){
    result.Clear();
    if(!CanRequestNative(NativeAction::Party,expected))return "Consulta requer perfil online.";
    std::ifstream flag(dir+"enable-native-privileges.txt");
    std::string line;
    if(!std::getline(flag,line)||(line!="ENABLE-NATIVE-PRIVILEGES"&&line!="ENABLE-NATIVE-PRIVILEGES\r"))
        return "Consulta experimental desativada.";
    if(XamIsUIActive())return "Interface Xbox ocupada.";
    HANDLE module=0;void* address=nullptr;
    if(XexGetModuleHandle("xam.xex",&module)!=0||!module||
       XexGetProcedureAddress(module,0x212u,&address)!=0||!address)return "Consulta nao exportada.";
    const auto user=static_cast<std::uint32_t>(expected.controller);
    const auto verify=[&](){USER_SIGNIN_INFO info{};return XamUserGetSigninInfo(user,0,&info)==0&&
        info.xuid==expected.identity&&info.signin_state==2&&XamUserGetSigninState(user)==2;};
    if(!verify())return "Perfil alterado.";
    using Check=std::uint32_t(*)(std::uint32_t,std::uint32_t,std::uint32_t*);
    PrivilegeValues values{};
    for(std::size_t i=0;i<PrivilegeIds.size();++i){
        std::uint32_t value=0xffffffffu;
        const auto rc=reinterpret_cast<Check>(address)(user,PrivilegeIds[i],&value);
        if(!verify())return "Perfil alterado durante consulta.";
        values[i]=DecodePrivilege(rc,value);
    }
    if(!result.Set(expected,expected,values,now))return "Resultado invalido.";
    return "Permissoes consultadas; o Xbox controla o acesso final.";
}
}
