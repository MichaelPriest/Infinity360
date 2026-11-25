#pragma once
#include "StoreCatalog.h"
#include "FileWorkspace.h"
namespace StoreInstallPath {
inline std::string Category(const StoreItem& item){
    if(item.category=="homebrew")return "Homebrew";
    if(item.category=="app")return "Apps";
    if(item.category=="emulator")return "Emulators";
    if(item.category=="dashboard"&&item.id.size()==21&&item.id.compare(0,9,"infinity-")==0&&item.id.substr(9).find_first_not_of("0123456789abcdef")==std::string::npos)return "DashboardVersions";
    return "";
}
inline std::string Folder(const std::string& root,const StoreItem& item){
    const std::string category=Category(item);
    if(category.empty()||item.id.empty()||item.id.size()>32||
       item.id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-")!=std::string::npos)return "";
    return FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(root,"UserFiles"),category),item.id);
}
}
