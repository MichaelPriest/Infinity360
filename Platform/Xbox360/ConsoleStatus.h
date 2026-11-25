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
    static ConsoleStatus Read(){
        ConsoleStatus result;char buffer[32];const auto now=std::time(nullptr);
        if(auto* local=std::localtime(&now)){if(std::strftime(buffer,sizeof buffer,"%H:%M",local))result.clock=buffer;}
        if(result.clock.empty())result.clock="--:--";
#ifdef __XSDK__
        const char* langs[]={"Desconhecido","English","Japanese","German","French","Spanish","Italian","Korean","Chinese (traditional)","Portuguese","Chinese (simplified)","Polish","Russian"};
        const auto lang=xsdk_system_language();result.language=lang<13?langs[lang]:"Desconhecido";
        result.country=std::to_string(xsdk_system_country());
        for(unsigned i=0;i<4;++i){
            char name[XSDK_XAM_GAMERTAG_BYTES]{};
            if(XamUserGetSigninState(i)&&XamUserGetName(i,name,sizeof name)==ERROR_SUCCESS){name[sizeof name-1]=0;result.users[i]=name;}
        }
#else
        result.language="Provider indisponivel";result.country="-";
#endif
        return result;
    }
};
