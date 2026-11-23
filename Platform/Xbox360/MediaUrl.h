#pragma once
#include <string>

struct MediaUrl {
    std::string host, target;
    unsigned short port;
    bool secure;
    bool Parse(const std::string& address);
    bool NeedsConverter() const;
};
