#pragma once
#include "AvatarPreview.h"
#include <xsdk/kernel.h>
namespace InfinitySocial {
inline AvatarCapabilities ProbeAvatarExports(){
    AvatarCapabilities out;
    HANDLE module=0;
    if(XexGetModuleHandle("xam.xex",&module)!=0||!module)return out;
    out.checked=true;
    for(std::size_t i=0;i<AvatarExports.size();++i){
        void* address=nullptr;
        out.found[i]=XexGetProcedureAddress(module,AvatarExports[i],&address)==0&&address;
    }
    return out;
}
}
