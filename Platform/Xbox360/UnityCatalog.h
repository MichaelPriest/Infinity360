#pragma once
#include "ContentCatalog.h"
#include <string>
#include <vector>
struct UnityUpdate {
    unsigned id=0;
    std::string version;
    unsigned long long bytes=0;
};
namespace UnityCatalog {
std::string UpdatesUrl(const XboxExecutionInfo& info);
// Metadata only. No URL from a server response is executed or fetched.
bool ParseUpdates(const std::string& json,std::vector<UnityUpdate>& updates);
}
