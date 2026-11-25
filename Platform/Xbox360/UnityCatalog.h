#pragma once
#include "ContentCatalog.h"
#include <string>
#include <vector>
struct UnityUpdate {
    unsigned id=0;
    std::string version,url;
    unsigned long long bytes=0;
};
namespace UnityCatalog {
std::string UpdatesUrl(const XboxExecutionInfo& info);
// Returned file URLs must pass the explicit XboxUnity host policy before use.
std::string FileUrl(const std::string& url);
std::string Md5Url(unsigned id);
std::string CoversUrl(unsigned title);
std::string CoverUrl(unsigned image);
bool ParseCovers(const std::string& json,unsigned title,std::vector<unsigned>& images);
bool ParseMd5(const std::string& json,std::string& digest);
bool ParseUpdates(const std::string& json,std::vector<UnityUpdate>& updates);
}
