#pragma once
#include "ContentCatalog.h"
#include "FileWorkspace.h"
#include <functional>
namespace ContentInstall {
struct Request {
    XboxContentPackage package{};
    std::string filename,sha256,staged,backupSlot;
    unsigned long long bytes=0;
};
// Preserve the supplied legacy cache name; never synthesize one.
inline bool LegacyFilename(const std::string& name){
    if(name.size()<8||name.size()>42||name.compare(0,3,"TU_")!=0)return false;
    const auto dot=name.find('.',3);
    if(dot==std::string::npos||dot<=3||dot+1>=name.size()||name.find('.',dot+1)!=std::string::npos)return false;
    const std::string chars="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    return name.substr(3,dot-3).find_first_not_of(chars+"_")==std::string::npos&&
           name.substr(dot+1).find_first_not_of(chars)==std::string::npos&&FileWorkspace::ValidName(name);
}
inline bool Filename(unsigned type,const std::string& name){
    if(type==0xb0000u)return LegacyFilename(name)||(name.size()==19&&name.compare(0,2,"tu")==0&&name[10]=='_'&&
        name.substr(2,8).find_first_not_of("0123456789abcdefABCDEF")==std::string::npos&&
        name.substr(11).find_first_not_of("0123456789abcdefABCDEF")==std::string::npos);
    return type==2u&&name.size()>=16&&name.size()<=42&&name.find_first_not_of("0123456789abcdefABCDEF")==std::string::npos;
}
// Missing base-version metadata is common in legacy packages. Never claim
// compatibility if the known base versions disagree; missing values remain unknown.
inline bool CompatibleUpdate(const XboxExecutionInfo& game,const XboxContentPackage& pkg){
    return game.titleId && game.mediaId && pkg.titleId==game.titleId &&
        pkg.mediaId==game.mediaId && pkg.contentType==0xb0000u && pkg.volumeType==0 &&
        (!game.baseVersion || !pkg.baseVersion || game.baseVersion==pkg.baseVersion);
}
bool Read(const FileWorkspace& ws,const std::string& root,const XboxExecutionInfo& game,Request& out,std::string& error);
// A downloaded TU is eligible only with its locally generated receipt and exact game identity.
bool ReadStaged(const FileWorkspace& ws,const std::string& root,const XboxExecutionInfo& game,const std::string& staged,Request& out,std::string& error);
// Reinstall a verified backup using its original, validated package filename.
bool ReadBackup(const FileWorkspace& ws,const std::string& root,const XboxExecutionInfo& game,const std::string& slot,Request& out,std::string& error);
bool Install(const FileWorkspace& ws,const std::string& root,const XboxExecutionInfo& game,const Request& selected,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error);
}
