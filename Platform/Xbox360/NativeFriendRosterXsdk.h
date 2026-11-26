#pragma once
#include "FriendRoster.h"
#include <xsdk/kernel.h>
#include <fstream>
#include <cstdint>

namespace InfinitySocial {
// Experimental XLiveBase ABI, corroborated by independent emulator/recomp
// implementations. Manual opt-in only, no service credentials or writes.
inline std::string ReadNativeFriendRoster(const Header& expected,const std::string& dir,FriendRoster& roster){
    roster.Clear();
    if(!CanRequestNative(NativeAction::Friends,expected))return "Amigos requerem perfil online.";
    std::ifstream flag(dir+"enable-native-friend-roster.txt");
    std::string line;
    if(!std::getline(flag,line)||(line!="ENABLE-NATIVE-FRIEND-ROSTER"&&line!="ENABLE-NATIVE-FRIEND-ROSTER\r"))
        return "Lista experimental desativada. X abre lista oficial.";
    if(XamIsUIActive())return "Outra interface Xbox esta aberta.";
    const auto user=static_cast<std::uint32_t>(expected.controller);
    const auto verify=[&](){
        USER_SIGNIN_INFO info{};
        return XamUserGetSigninInfo(user,0,&info)==0&&info.xuid==expected.identity&&
            info.signin_state==2&&XamUserGetSigninState(user)==2;
    };
    if(!verify())return "O perfil mudou.";
    HANDLE module=0;void* address=nullptr;
    if(XexGetModuleHandle("xam.xex",&module)!=0||!module||
       XexGetProcedureAddress(module,0x1f4u,&address)!=0||!address)return "Servico de lista indisponivel.";
    // PPC native argument list: size, padding, 64-bit address of value.
    struct alignas(8) Arg {std::uint32_t bytes,pad;std::uint64_t address;};
    static_assert(sizeof(Arg)==16,"XLiveBase argument size");
    static_assert(sizeof(HANDLE)==4,"Xbox 360 handle size");
    std::uint32_t index=user,first=0,count=MaxFriends,bytes=0;
    HANDLE handle=0;
    const auto arg=[](void* value)->Arg{return {4,0,static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(value))};};
    Arg args[]={arg(&index),arg(&first),arg(&count),arg(&bytes),arg(&handle)};
    using Call=std::uint32_t(*)(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t);
    const auto status=reinterpret_cast<Call>(address)(0xfcu,0x58020u,0u,
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(args)));
    if(status!=0||!handle||(std::uintptr_t)handle==0xffffffffu)return "Servico nao retornou enumerador valido.";
    struct CloseHandle {HANDLE h;~CloseHandle(){NtClose(h);}} close{handle};
    if(!bytes||bytes>MaxFriends*FriendRecordBytes||bytes%FriendRecordBytes||!verify())
        return "Tamanho de lista invalido ou perfil alterado.";
    alignas(8) std::array<unsigned char,MaxFriends*FriendRecordBytes> data{};
    std::uint32_t items=0;
    const auto rc=XamEnumerate(handle,0,data.data(),bytes,&items,nullptr);
    if((rc!=0&&!(rc==18&&items==0))||items>bytes/FriendRecordBytes||!verify())
        return "Consulta recusada ou perfil alterado.";
    if(!roster.Set(expected,expected,data.data(),bytes,items))return "Formato de lista nao reconhecido.";
    return items?"Lista consultada no sistema.":"Sistema retornou lista vazia.";
}
}
