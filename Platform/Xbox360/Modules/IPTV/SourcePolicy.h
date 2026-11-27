#pragma once
#include "../../StoreHostPolicy.h"
#include "../../TvCatalog.h"
#include <cstdio>
namespace InfinityIptv {
// Explicit shipped preference; missing or invalid file retains restricted mode.
inline bool OpenMode(const std::string& root){
    std::ifstream in((root+"infinity360-iptv-network.txt").c_str(),std::ios::binary);
    char buf[32];in.read(buf,sizeof buf);
    if(in.bad()||in.gcount()>=static_cast<std::streamsize>(sizeof buf))return false;
    const std::string value(buf,static_cast<size_t>(in.gcount()));
    return value=="OPEN"||value=="OPEN\n"||value=="OPEN\r\n";
}
inline bool NetworkAllowed(const std::string& address,const std::string& root){
    MediaUrl url;if(!url.Parse(address))return true; // local/reference format validated separately
    if(address.find('#')!=std::string::npos)return false;
    if(OpenMode(root))return true; // HTTP/HTTPS only, TLS verification remains in readers
    if(url.secure)return StoreHostPolicy::Allowed(address,root);
    std::ifstream relay((root+"infinity360-iptv-relay.txt").c_str());std::string allowed;
    if(!std::getline(relay,allowed)||allowed!=url.host||url.host.find_first_not_of("0123456789.")!=std::string::npos)return false;
    unsigned a,b,c,d;char tail;
    if(std::sscanf(url.host.c_str(),"%u.%u.%u.%u%c",&a,&b,&c,&d,&tail)!=4||a>255||b>255||c>255||d>255||
       !(a==10||(a==172&&b>=16&&b<=31)||(a==192&&b==168)))return false;
    return url.host==std::to_string(a)+"."+std::to_string(b)+"."+std::to_string(c)+"."+std::to_string(d);
}
inline size_t ExternalReferences(const std::vector<TvItem>& items){
    size_t count=0;for(const auto& item:items)if(tvLower(item.uri.substr(0,12))=="brazucaplay:")++count;return count;
}
}
