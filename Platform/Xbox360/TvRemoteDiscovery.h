#pragma once
#include "TvRemoteProtocol.h"
#include "StoreSha256.h"
namespace TvRemote {
inline std::string DiscoveryMessage(const std::string& nonce,const std::string& ip){
    return "INFINITY360/1 "+nonce+" "+ip;
}
// HMAC-SHA256 with the same 32 ASCII hexadecimal key bytes on both peers.
inline std::string DiscoveryMac(const std::string& secret,const std::string& message){
    if(!ValidSecret(secret))return "";
    unsigned char innerPad[64],outerPad[64];
    for(size_t i=0;i<64;++i){const unsigned char k=i<secret.size()?secret[i]:0;innerPad[i]=k^0x36;outerPad[i]=k^0x5c;}
    StoreSha256 inner;inner.Update(innerPad,64);inner.Update(reinterpret_cast<const unsigned char*>(message.data()),message.size());
    const auto hex=inner.Finish();unsigned char digest[32];
    auto nibble=[](char c){return c<='9'?c-'0':c-'a'+10;};
    for(size_t i=0;i<32;++i)digest[i]=static_cast<unsigned char>(nibble(hex[i*2])*16+nibble(hex[i*2+1]));
    StoreSha256 outer;outer.Update(outerPad,64);outer.Update(digest,32);return outer.Finish();
}
inline std::string DiscoveryReply(const std::string& request,const std::string& ip,const std::string& secret){
    const std::string prefix="INFINITY360-DISCOVER/1 ";
    if(request.compare(0,prefix.size(),prefix)!=0)return "";
    const auto nonce=request.substr(prefix.size());
    if(!ValidSecret(nonce)||!ValidSecret(secret))return "";
    const auto message=DiscoveryMessage(nonce,ip);
    return message+" "+DiscoveryMac(secret,message);
}
}
