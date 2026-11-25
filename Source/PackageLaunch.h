#pragma once
#include "ContentCatalog.h"
#include "XexLaunchCheck.h"
#include <cstdio>
#include <fstream>
namespace PackageLaunch {
struct Ticket {
    XboxContentPackage package{};
    unsigned long long bytes=0,fingerprint=0;
};
inline bool Supported(const XboxContentPackage& p){
    return p.titleId&&!XexLaunchCheck::OwnTitle(p.titleId)&&
        ((p.contentType==0x7000u&&p.volumeType==1u&&p.dataFileCount>0&&p.dataFileCount<=1024)||
         (p.contentType==0xd0000u&&p.volumeType==0u&&p.dataFileCount==0));
}
inline bool Capture(const std::string& path,Ticket& out){
    out={};XboxContentPackage p{};
    if(!ContentCatalog::ReadHeader(path,p)||!Supported(p)||
        ContentCatalog::CheckPackage(path).status!=XboxPackageCheck::StructurePresent)return false;
    std::ifstream file(path.c_str(),std::ios::binary|std::ios::ate);
    const auto size=file.tellg();if(!file||size<0x1711)return false;
    unsigned long long hash=14695981039346656037ULL;
    auto sample=[&](std::streamoff offset){
        unsigned char buffer[4096];file.seekg(offset,std::ios::beg);
        if(!file.read(reinterpret_cast<char*>(buffer),sizeof buffer))return false;
        for(unsigned char b:buffer){hash^=b;hash*=1099511628211ULL;}return true;
    };
    if(!sample(0)||!sample(static_cast<std::streamoff>(size)-4096))return false;
    if(p.volumeType==1){
        for(unsigned i=0;i<p.dataFileCount;++i){
            char name[16];std::sprintf(name,"Data%04u",i);
            std::ifstream part((path+".data/"+name).c_str(),std::ios::binary|std::ios::ate);
            if(!part||part.tellg()<=0)return false;
            const auto bytes=static_cast<unsigned long long>(part.tellg());
            for(unsigned j=0;j<8;++j){hash^=(bytes>>(j*8))&255;hash*=1099511628211ULL;}
        }
    }
    out.package=p;out.bytes=static_cast<unsigned long long>(size);out.fingerprint=hash;return true;
}
inline bool Same(const Ticket& a,const Ticket& b){
    return a.bytes&&a.bytes==b.bytes&&a.fingerprint==b.fingerprint&&a.package.titleId==b.package.titleId&&
        a.package.mediaId==b.package.mediaId&&a.package.contentType==b.package.contentType&&
        a.package.version==b.package.version&&a.package.volumeType==b.package.volumeType&&
        a.package.dataFileCount==b.package.dataFileCount&&a.package.path==b.package.path;
}
}
