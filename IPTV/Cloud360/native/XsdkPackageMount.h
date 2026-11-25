#pragma once
#include "PackageLaunch.h"
#include "XsdkPaths.h"
#include "xsdk/kernel.h"
#include "xsdk/xam.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>
// Experimental retail XAM provider. Never edit licenses or container headers.
// Resolve optional export at runtime, leaving all authentication to XAM.
class XsdkPackageMount {
    static constexpr const char* MountRoot="infpkg";
    bool linked_=false,mounted_=false;
    static ANSI_STRING Ansi(const char* s){
        ANSI_STRING a{};a.length=static_cast<uint16_t>(std::strlen(s));a.maximum_length=a.length+1;a.pointer=s;return a;
    }
public:
    static constexpr const char* Executable="infpkg:/default.xex";
    XsdkPackageMount()=default;
    XsdkPackageMount(const XsdkPackageMount&)=delete;
    XsdkPackageMount& operator=(const XsdkPackageMount&)=delete;
    ~XsdkPackageMount(){Close();}
    void Close(){
        if(mounted_){(void)XamContentClose(MountRoot,nullptr);mounted_=false;}
        if(linked_){const auto link=Ansi("\\??\\infsource:");(void)ObDeleteSymbolicLink(&link);linked_=false;}
    }
    bool Open(const std::string& path,const PackageLaunch::Ticket& expected,std::string& error){
        if(linked_||mounted_){error="Montagem ja aberta.";return false;}
        PackageLaunch::Ticket current;
        if(!PackageLaunch::Capture(path,current)||!PackageLaunch::Same(expected,current)){error="Pacote alterado ou incompleto.";return false;}
        unsigned user=4;for(unsigned i=0;i<4;++i)if(XamUserGetSigninState(i)){user=i;break;}
        if(user==4){error="Entre em um perfil Xbox autorizado antes de montar.";return false;}
        HANDLE module=0;void* address=nullptr;
        if(XexGetModuleHandle("xam.xex",&module)!=ERROR_SUCCESS||!NT_SUCCESS(XexGetProcedureAddress(module,0x269,&address))||!address){
            error="XamContentOpenFile indisponivel neste kernel.";return false;
        }
        // XAM accepts a title drive as the input path. The XSDK resolves HDD/USB
        // aliases to NT device paths; give those a temporary title drive as well.
        const auto normalized=XsdkPaths::Normalize(path);
        if(normalized.empty()){error="Caminho do pacote invalido.";return false;}
        std::string source;
        if(normalized.compare(0,6,"game:/")==0){
            source=normalized;
            std::replace(source.begin(),source.end(),'/','\\');
        }else{
            char* raw=realpath(normalized.c_str(),nullptr);
            if(!raw){error="Caminho fisico indisponivel.";return false;}
            std::string physical(raw);std::free(raw);
            std::replace(physical.begin(),physical.end(),'/','\\');
            const auto slash=physical.find_last_of('\\');
            if(slash==std::string::npos||physical.compare(0,8,"\\Device\\")!=0||physical.size()>240){error="Caminho fisico invalido.";return false;}
            const auto folder=physical.substr(0,slash),name=physical.substr(slash+1);
            const auto link=Ansi("\\??\\infsource:"),target=Ansi(folder.c_str());
            if(!NT_SUCCESS(ObCreateSymbolicLink(&link,&target))){error="Nao foi possivel reservar unidade temporaria.";return false;}
            linked_=true;
            source="infsource:\\"+name;
        }
        using OpenFile=uint32_t(*)(uint32_t,const char*,const char*,uint32_t,uint32_t*,uint32_t*,XAM_OVERLAPPED*);
        auto open=reinterpret_cast<OpenFile>(address);
        uint32_t disposition=0,license=0;
        // 0x43 is an existing-package mount observed in public XAM call examples.
        // No license override flags, no create/overwrite disposition.
        const uint32_t result=open(user,MountRoot,source.c_str(),0x43,&disposition,&license,nullptr);
        if(result!=ERROR_SUCCESS){Close();error="XAM recusou montagem: "+std::to_string(result);return false;}
        mounted_=true;
        XboxExecutionInfo info{};std::ifstream file(Executable,std::ios::binary);
        if(!XexLaunchCheck::Header(file)||!ContentCatalog::ReadExecutionInfo(Executable,info)||
           info.titleId!=current.package.titleId||XexLaunchCheck::OwnTitle(info.titleId)){
            Close();error="default.xex interno ausente ou Title ID divergente.";return false;
        }
        return true;
    }
};
