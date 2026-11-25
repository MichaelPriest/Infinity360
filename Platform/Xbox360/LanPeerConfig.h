#pragma once
#include "LanPeerProbe.h"
#include "SavedFile.h"
#include <array>
#include <fstream>
#include <string>

namespace LanPeerConfig {
using Octets=std::array<unsigned,4>;
inline bool Parse(const std::string& ip,Octets& result){
    if(!LanPeerProbe::Private(ip))return false;
    Octets next{};unsigned i=0;
    for(char c:ip){if(c=='.')++i;else next[i]=next[i]*10+unsigned(c-'0');}
    result=next;return true;
}
inline std::string Format(const Octets& a){
    return std::to_string(a[0])+"."+std::to_string(a[1])+"."+std::to_string(a[2])+"."+std::to_string(a[3]);
}
inline bool Change(Octets& a,unsigned index,int delta){
    if(index>=4||(delta!=-1&&delta!=1))return false;
    if(a[index]>255)return false;
    a[index]=(a[index]+(delta==1?1:255))%256;
    return true;
}
inline bool Read(const std::string& path,Octets& result){
    std::ifstream in(savedFile(path).c_str(),std::ios::binary);
    char raw[32]{};
    if(!in)return false;
    in.read(raw,sizeof raw);
    const auto n=in.gcount();
    if(in.bad()||n<=0||n>=static_cast<std::streamsize>(sizeof raw))return false;
    std::string ip(raw,static_cast<size_t>(n));
    if(!ip.empty()&&ip.back()=='\n')ip.pop_back();
    if(!ip.empty()&&ip.back()=='\r')ip.pop_back();
    return Parse(ip,result);
}
inline bool Save(const std::string& path,const Octets& a){
    const auto ip=Format(a);if(!LanPeerProbe::Private(ip))return false;
    std::ofstream out((path+".tmp").c_str(),std::ios::binary|std::ios::trunc);
    out<<ip<<"\n";out.close();return bool(out)&&replaceSavedFile(path);
}
}
