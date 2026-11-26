#pragma once
#include "AvatarPreview.h"
#include <xsdk/kernel.h>
#include <fstream>

namespace InfinitySocial {
// Retrieve the avatar editor's cached full-body PNG, not a live 3D scene.
// Opt-in; both export ABIs and physical texture byte order need validation.
inline std::string ReadNativeAvatarBody(const Header& expected,const std::string& dir,AvatarPreview& preview){
    preview.Clear();
    if(!CanRequestNative(NativeAction::GamerCard,expected))return "Avatar requer perfil ativo.";
    std::ifstream flag(dir+"enable-native-avatar-body.txt");
    std::string line;
    if(!std::getline(flag,line)||(line!="ENABLE-NATIVE-AVATAR-BODY"&&line!="ENABLE-NATIVE-AVATAR-BODY\r"))
        return "Corpo inteiro experimental desativado.";
    if(XamIsUIActive())return "Interface Xbox ocupada.";
    HANDLE module=0;void* readAddress=nullptr;void* decodeAddress=nullptr;
    if(XexGetModuleHandle("xam.xex",&module)!=0||!module||
        XexGetProcedureAddress(module,0x2efu,&readAddress)!=0||!readAddress||
        XexGetProcedureAddress(module,0x2f4u,&decodeAddress)!=0||!decodeAddress)
        return "Leitura/decoder de avatar indisponivel.";
    const auto user=static_cast<std::uint32_t>(expected.controller);
    const auto state=expected.state==SignIn::Online?2u:1u;
    const auto verify=[&](){USER_SIGNIN_INFO info{};return XamUserGetSigninInfo(user,0,&info)==0&&
        info.xuid==expected.identity&&info.signin_state==state&&XamUserGetSigninState(user)==state;};
    if(!verify())return "Perfil alterado.";
    using ReadTile=std::uint32_t(*)(std::uint32_t,std::uint32_t,std::uint64_t,std::uint32_t,void*,std::uint32_t*,void*);
    using Decode=std::int32_t(*)(const void*,std::uint32_t,void*,std::uint32_t,std::uint32_t);
    // Fixed maximum capacity observed in the editor. No arbitrary paths or XUIDs.
    std::vector<unsigned char> encoded(AvatarPngLimit,0);
    std::uint32_t bytes=static_cast<std::uint32_t>(encoded.size());
    const auto rc=reinterpret_cast<ReadTile>(readAddress)(15u,0xfffe0851u,0u,user,encoded.data(),&bytes,nullptr);
    if(rc!=0||bytes>encoded.size()||!verify())return "Imagem de corpo inteiro indisponivel.";
    unsigned w=0,h=0;
    if(!InspectAvatarPng(encoded.data(),bytes,w,h))return "PNG do avatar fora dos limites aceitos.";
    const unsigned pitch=(w*4u+127u)&~127u;
    std::vector<unsigned char> raw(std::size_t(pitch)*h+127,0);
    auto* aligned=reinterpret_cast<unsigned char*>((reinterpret_cast<std::uintptr_t>(raw.data())+127u)&~std::uintptr_t(127u));
    const auto status=reinterpret_cast<Decode>(decodeAddress)(encoded.data(),bytes,aligned,pitch,h);
    if(status!=0||!verify())return "Sistema nao decodificou o avatar.";
    std::vector<unsigned char> rgba;
    if(!DecodeAvatarArgb(aligned,std::size_t(pitch)*h,w,h,pitch,rgba)||
       !preview.Set(expected,expected,w,h,rgba))return "Textura de avatar invalida.";
    return "Imagem oficial de corpo inteiro consultada (estatica).";
}
}
