#pragma once
#include <string>
#include <ctime>
#ifdef __XSDK__
#include "xsdk/xam.h"
#include "xsdk/system.h"
#endif
struct ConsoleStatus {
    std::string clock,language,country;
    std::string users[4];
    unsigned signin[4]={};
    bool profileProvider=false;
    static const char* StateLabel(bool available,unsigned state){
        if(!available)return "Provider indisponivel";
        if(state==0)return "Sem sessao";
        if(state==1)return "Sessao local";
        if(state==2)return "Sessao online (XAM)";
        return "Estado desconhecido";
    }
    static ConsoleStatus Read(){
        ConsoleStatus result;char buffer[32];const auto now=std::time(nullptr);
        if(auto* local=std::localtime(&now)){if(std::strftime(buffer,sizeof buffer,"%H:%M",local))result.clock=buffer;}
        if(result.clock.empty())result.clock="--:--";
#ifdef __XSDK__
        const char* langs[]={"Desconhecido","English","Japanese","German","French","Spanish","Italian","Korean","Chinese (traditional)","Portuguese","Chinese (simplified)","Polish","Russian"};
        const auto lang=xsdk_system_language();result.language=lang<13?langs[lang]:"Desconhecido";
        result.country=std::to_string(xsdk_system_country());
        result.profileProvider=true;
        for(unsigned i=0;i<4;++i){
            char name[XSDK_XAM_GAMERTAG_BYTES]{};
            result.signin[i]=XamUserGetSigninState(i);
            if(result.signin[i]&&XamUserGetName(i,name,sizeof name)==ERROR_SUCCESS){name[sizeof name-1]=0;result.users[i]=name;}
        }
#else
        result.language="Provider indisponivel";result.country="-";
#endif
        return result;
    }
};
