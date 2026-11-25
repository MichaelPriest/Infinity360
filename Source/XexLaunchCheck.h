#pragma once
#include "XsdkPaths.h"
#include <cstdint>
#include <istream>
#include <string>

namespace XexLaunchCheck {
inline uint32_t Be32(const unsigned char* p){
    return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];
}
// Validate the container's outer bounds only. The console remains responsible
// for authentication, execution permissions, decryption and compatibility.
inline bool Header(std::istream& file) {
    file.seekg(0,std::ios::end);
    const std::streamoff size=file.tellg();
    if(size<4096)return false;
    file.seekg(0,std::ios::beg);
    unsigned char h[24];
    if(!file.read(reinterpret_cast<char*>(h),sizeof h))return false;
    if(h[0]!='X'||h[1]!='E'||h[2]!='X'||h[3]!='2')return false;
    const uint32_t image=Be32(h+8),security=Be32(h+16),entries=Be32(h+20);
    if(entries>4096)return false;
    const uint32_t tableEnd=24u+entries*8u;
    return image>=tableEnd && static_cast<uint64_t>(image)<static_cast<uint64_t>(size)
        && security>=tableEnd && security<=image && image-security>=4;
}
// Bounded identity sample, not authentication of an executable.
struct Ticket {uint64_t bytes=0,fingerprint=0;};
inline bool Capture(std::istream& file,Ticket& out){
    out={};if(!Header(file))return false;
    file.clear();file.seekg(0,std::ios::end);const auto size=file.tellg();
    if(size<4096)return false;
    uint64_t hash=14695981039346656037ULL;
    auto sample=[&](std::streamoff offset){
        file.seekg(offset,std::ios::beg);unsigned char data[4096];
        if(!file.read(reinterpret_cast<char*>(data),sizeof data))return false;
        for(unsigned char b:data){hash^=b;hash*=1099511628211ULL;}return true;
    };
    if(!sample(0)||!sample(static_cast<std::streamoff>(size)-4096))return false;
    out.bytes=static_cast<uint64_t>(size);out.fingerprint=hash;return true;
}
inline bool Same(const Ticket& a,const Ticket& b){return a.bytes>=4096&&a.bytes==b.bytes&&a.fingerprint==b.fingerprint;}
inline bool OwnTitle(unsigned title){return (title>=0x494E4631u&&title<=0x494E4633u)||title==0x41555831u||title==0x43333630u||title==0x43333631u;}
inline std::string Path(const std::string& raw) {
    const std::string path=XsdkPaths::Normalize(raw);
    const std::string key=XsdkPaths::Key(path);
    if(key.size()<5||key.substr(key.size()-4)!=".xex")return "";
    const std::string name=key.substr(key.find_last_of('/')+1);
    if(name=="infinity_menu_preview.xex"||name=="infinity_canvas_probe.xex"||name=="infinity_core_probe.xex"||name=="aurorax_gpu_probe.xex")return "";
    return path;
}
}
