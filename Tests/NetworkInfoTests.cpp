#include "NetworkInfo.h"
#include <cassert>
#include <string>

int main() {
    const unsigned char lan[]={192,168,1,22};
    const unsigned char zero[]={0,0,0,0};
    const unsigned char loop[]={127,0,0,1};
    const unsigned char multicast[]={224,0,0,1};
    const unsigned char broadcast[]={255,255,255,255};
    const unsigned char linkLocal[]={169,254,2,3};
    assert(NetworkInfo::IPv4(lan)=="192.168.1.22");
    assert(NetworkInfo::IPv4(linkLocal)=="169.254.2.3");
    assert(NetworkInfo::IPv4(zero).empty());
    assert(NetworkInfo::IPv4(loop).empty());
    assert(NetworkInfo::IPv4(multicast).empty());
    assert(NetworkInfo::IPv4(broadcast).empty());
    assert(NetworkInfo::IPv4(0).empty());
    assert(std::string(NetworkInfo::StateLabel(NetworkSnapshot::Pending)).find("Aguardando")!=std::string::npos);
    NetworkSnapshot result=NetworkInfo::Read();
    if(result.state==NetworkSnapshot::AddressReady)assert(!result.ipv4.empty());
    else assert(result.ipv4.empty());
#ifndef _XBOX
    assert(!result.ethernetKnown);
#endif
}
