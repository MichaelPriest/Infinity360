#pragma once
#include "ContentCatalog.h"
#include "GameScanner.h"
#include <algorithm>
#include <set>
namespace DiscSet {
struct Entry {std::string path,name,device;XboxExecutionInfo info{};};
// Do not infer disc membership from folder names or mix different base versions.
// Different copies of the same disc remain separate, never choose automatically.
inline bool Matches(const XboxExecutionInfo& a,const XboxExecutionInfo& b){
    return a.titleId&&a.titleId==b.titleId&&a.baseVersion==b.baseVersion&&a.discCount>1&&a.discCount==b.discCount&&
        a.discNumber>=1&&a.discNumber<=a.discCount&&b.discNumber>=1&&b.discNumber<=b.discCount;
}
inline bool ReadIdentity(const std::string& path,XboxExecutionInfo& info){
    if(ContentCatalog::ReadExecutionInfo(path,info))return true;
    XboxContentPackage package{};
    if(!ContentCatalog::ReadHeader(path,package)||package.contentType!=0x7000||package.volumeType!=1)return false;
    info={package.titleId,package.mediaId,package.version,package.baseVersion,package.discNumber,package.discCount};
    return true;
}
inline std::vector<Entry> Find(const std::vector<Game>& games,const XboxExecutionInfo& current){
    std::vector<Entry> out;std::set<std::string> seen;
    if(current.discCount<=1||!current.titleId)return out;
    for(const auto& game:games){
        XboxExecutionInfo info{};
        if(!ReadIdentity(game.executable,info)||!Matches(current,info)||!seen.insert(game.executable).second)continue;
        out.push_back({game.executable,game.name,game.device,info});
        if(out.size()==128)break;
    }
    std::sort(out.begin(),out.end(),[](const Entry& a,const Entry& b){
        return a.info.discNumber==b.info.discNumber?a.path<b.path:a.info.discNumber<b.info.discNumber;
    });
    return out;
}
}
