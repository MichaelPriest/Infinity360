#include "NetworkInfo.h"
#include "../Platform/Xbox360/LanPeerProbe.h"
#include "../Platform/Xbox360/LanPeerConfig.h"
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
    NetworkSnapshot flags;flags.SetLink(true,true,false,true,false);
    assert(flags.linkMbps==100&&flags.duplex=="Full");flags.SetLink(true,true,true,true,true);
    assert(!flags.linkMbps&&flags.duplex.empty());
    assert(LanPeerProbe::Private("192.168.1.20")&&LanPeerProbe::Private("172.16.0.20"));
    assert(!LanPeerProbe::Private("8.8.8.8")&&!LanPeerProbe::Private("192.168.01.20")&&!LanPeerProbe::Private("192.168.1.255"));
    LanPeerConfig::Octets ip{};
    assert(LanPeerConfig::Parse("192.168.1.254",ip));
    assert(!LanPeerConfig::Parse("192.168.1.255",ip));
    assert(LanPeerConfig::Format(ip)=="192.168.1.254");
    assert(LanPeerConfig::Change(ip,3,1)&&LanPeerConfig::Change(ip,3,1)&&ip[3]==0);
    assert(!LanPeerConfig::Change(ip,4,1));
    assert(LanPeerConfig::Change(ip,3,-1)&&ip[3]==255);
    unsigned char packet[12];bool reply=false;uint32_t nonce=0;
    LanPeerProbe::Packet(packet,true,0x12345678);
    assert(LanPeerProbe::Decode(packet,sizeof packet,reply,nonce)&&reply&&nonce==0x12345678);
    assert(!LanPeerProbe::Decode(packet,11,reply,nonce));
    packet[6]=1;assert(!LanPeerProbe::Decode(packet,sizeof packet,reply,nonce));
    NetworkSnapshot result=NetworkInfo::Read();
    if(result.state==NetworkSnapshot::AddressReady)assert(!result.ipv4.empty());
    else assert(result.ipv4.empty());
#ifndef _XBOX
    assert(!result.ethernetKnown);
#endif
}
