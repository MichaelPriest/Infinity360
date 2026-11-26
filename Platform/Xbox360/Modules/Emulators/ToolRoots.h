#pragma once
#include <string>
#include <vector>
#include "GameScanner.h"
namespace InfinityToolRoots {
inline std::string Join(const std::string& root,const std::string& name){
    if(root.empty())return name;
    if(root.back()=='/'||root.back()=='\\')return root+name;
    return root+(root.find('\\')==std::string::npos?"/":"\\")+name;
}
inline void Append(std::vector<std::string>& roots,const std::string& app,const std::string& category){
    if(category!="Apps"&&category!="Emulators"&&category!="Homebrew")return;
    auto add=[&](const std::string& path){
        for(const auto& old:roots)if(GameScanner::SameRoot(old,path))return;
        roots.push_back(path);
    };
    add(Join(app,category));
    auto base=app;
    while(!base.empty()&&(base.back()=='/'||base.back()=='\\'))base.pop_back();
    const auto sep=base.find_last_of("/\\");
    auto leaf=base.substr(sep==std::string::npos?0:sep+1);
    for(char& ch:leaf)if(ch>='A'&&ch<='Z')ch=char(ch-'A'+'a');
    if(leaf=="menupreview")add(Join(sep==std::string::npos?"":base.substr(0,sep+1),category));
    else add(Join(Join(app,"MenuPreview"),category));
}
}
