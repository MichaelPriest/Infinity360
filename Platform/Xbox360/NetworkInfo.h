#pragma once
#include <string>

struct NetworkSnapshot {
    enum State { Unsupported, Error, Pending, NoAddress, AddressReady };
    State state;
    bool ethernetKnown, ethernetActive;
    std::string ipv4;
    NetworkSnapshot():state(Unsupported),ethernetKnown(false),ethernetActive(false) {}
};
class NetworkInfo {
public:
    static NetworkSnapshot Read();
    static std::string IPv4(const unsigned char* bytes);
    static bool UsableIPv4(const unsigned char* bytes);
    static const char* StateLabel(NetworkSnapshot::State state);
};
