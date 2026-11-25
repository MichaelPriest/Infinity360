#pragma once
#include <string>

struct NetworkSnapshot {
    enum State { Unsupported, Error, Pending, NoAddress, AddressReady };
    State state;
    bool ethernetKnown, ethernetActive;
    std::string ipv4;
    unsigned linkMbps=0;
    std::string duplex;
    void SetLink(bool active,bool mb100,bool mb10,bool full,bool half){
        ethernetKnown=true;ethernetActive=active;
        linkMbps=active&&(mb100!=mb10)?(mb100?100:10):0;
        duplex=active&&(full!=half)?(full?"Full":"Half"):"";
    }
    NetworkSnapshot():state(Unsupported),ethernetKnown(false),ethernetActive(false) {}
};
class NetworkInfo {
public:
    static NetworkSnapshot Read();
    static std::string IPv4(const unsigned char* bytes);
    static bool UsableIPv4(const unsigned char* bytes);
    static const char* StateLabel(NetworkSnapshot::State state);
};
