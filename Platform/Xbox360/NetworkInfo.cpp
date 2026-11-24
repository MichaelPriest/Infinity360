#include "NetworkInfo.h"
#include <cstdio>
#include <cstring>
#ifdef _XBOX
#include <xtl.h>
#include <winsockx.h>
#include <xnet.h>
#elif defined(__linux__)
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#endif

bool NetworkInfo::UsableIPv4(const unsigned char* b) {
    if(!b || b[0]==0 || b[0]==127 || b[0]>=224)return false;
    return true;
}
std::string NetworkInfo::IPv4(const unsigned char* b) {
    if(!UsableIPv4(b))return "";
    char text[16];
    std::sprintf(text,"%u.%u.%u.%u",unsigned(b[0]),unsigned(b[1]),
                 unsigned(b[2]),unsigned(b[3]));
    return text;
}
const char* NetworkInfo::StateLabel(NetworkSnapshot::State state) {
    switch(state) {
    case NetworkSnapshot::AddressReady:return "Endereco IPv4 local atribuido";
    case NetworkSnapshot::Pending:return "Aguardando endereco de rede";
    case NetworkSnapshot::NoAddress:return "Sem IPv4 local identificado";
    case NetworkSnapshot::Error:return "Falha ao consultar rede";
    default:return "Provider indisponivel nesta plataforma";
    }
}
NetworkSnapshot NetworkInfo::Read() {
    NetworkSnapshot result;
#ifdef _XBOX
    // Balanced initialization; no security bypass and no socket listener.
    XNetStartupParams params;
    std::memset(&params,0,sizeof(params));
    params.cfgSizeOfStruct=sizeof(params);
    if(XNetStartup(&params)!=0) {result.state=NetworkSnapshot::Error;return result;}
    result.ethernetKnown=true;
    result.ethernetActive=(XNetGetEthernetLinkStatus() & XNET_ETHERNET_LINK_ACTIVE)!=0;
    XNADDR address;
    std::memset(&address,0,sizeof(address));
    DWORD status=XNetGetTitleXnAddr(&address);
    // Bound the wait to 500 ms; manual refresh can retry later.
    for(int i=0;i<10 && status==XNET_GET_XNADDR_PENDING;++i) {
        Sleep(50);
        status=XNetGetTitleXnAddr(&address);
    }
    if(status==XNET_GET_XNADDR_PENDING)result.state=NetworkSnapshot::Pending;
    else {
        result.ipv4=IPv4(reinterpret_cast<const unsigned char*>(&address.ina));
        result.state=result.ipv4.empty()?NetworkSnapshot::NoAddress:NetworkSnapshot::AddressReady;
    }
    XNetCleanup();
#elif defined(__linux__)
    // Host diagnostics only: do not simulate Xbox Ethernet link flags.
    struct ifaddrs* list=0;
    if(getifaddrs(&list)!=0) {result.state=NetworkSnapshot::Error;return result;}
    result.state=NetworkSnapshot::NoAddress;
    for(struct ifaddrs* it=list;it;it=it->ifa_next) {
        if(!it->ifa_addr || it->ifa_addr->sa_family!=AF_INET ||
           !(it->ifa_flags & IFF_UP) || (it->ifa_flags & IFF_LOOPBACK))continue;
        const struct sockaddr_in* addr=reinterpret_cast<const struct sockaddr_in*>(it->ifa_addr);
        result.ipv4=IPv4(reinterpret_cast<const unsigned char*>(&addr->sin_addr));
        if(!result.ipv4.empty()) {result.state=NetworkSnapshot::AddressReady;break;}
    }
    freeifaddrs(list);
#endif
    return result;
}
