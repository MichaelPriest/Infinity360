#pragma once
#include "StoreCatalog.h"
#include "FileWorkspace.h"
#include <cstdio>
#include <cstdint>
namespace StorePartial {
inline std::string Path(const std::string& folder,const StoreItem& item){
    if(item.id.empty()||item.id.size()>32||item.id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-")!=std::string::npos||
       item.sha256.size()!=64||item.sha256.find_first_not_of("0123456789abcdef")!=std::string::npos||
       !item.bytes||item.bytes>512ULL*1024*1024)return "";
    uint32_t h=2166136261u;
    for(unsigned char ch:item.id){h^=ch;h*=16777619u;}
    char hex[9];std::sprintf(hex,"%08lx",static_cast<unsigned long>(h));
    // FATX max name 42. Reserved prefix hides partials from FTP and explorer edits.
    return FileWorkspace::Join(folder,std::string("AXCOPY-")+hex+item.sha256.substr(0,24)+".p");
}
}
