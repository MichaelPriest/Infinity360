#include "TrainerCatalog.h"
#include "GameScanner.h"
#include <algorithm>
#include <cstdio>
#include <set>
#include <fstream>
#include <map>
#include <sstream>
#ifndef _XBOX
#include <filesystem>
#endif

namespace {
std::string join(const std::string& parent,const std::string& child) {
    if(parent.empty())return child;
    if(parent[parent.size()-1]=='/' || parent[parent.size()-1]=='\\')return parent+child;
    return parent+(parent.find('\\')!=std::string::npos?"\\":"/")+child;
}
bool safe(const std::string& value) {
    if(value.empty() || value.size()>42 || value=="." || value==".." ||
       value.find("..")!=std::string::npos)return false;
    for(size_t i=0;i<value.size();++i) {
        const char ch=value[i];
        if(!((ch>='a'&&ch<='z')||(ch>='A'&&ch<='Z')||
            (ch>='0'&&ch<='9')||ch=='-'||ch=='_'||ch=='.'))return false;
    }
    return true;
}
bool normal(const std::string& path,bool directory) {
#ifndef _XBOX
    std::error_code ec;
    const std::filesystem::file_status st=std::filesystem::symlink_status(path,ec);
    return !ec && (directory?std::filesystem::is_directory(st):std::filesystem::is_regular_file(st));
#else
    (void)path;(void)directory;return true;
#endif
}
bool hex32(const std::string& value,unsigned& out) {
    if(value.size()!=8)return false;
    out=0;
    for(size_t i=0;i<8;++i) {
        const char ch=value[i];unsigned n;
        if(ch>='0'&&ch<='9')n=ch-'0';
        else if(ch>='A'&&ch<='F')n=ch-'A'+10;
        else if(ch>='a'&&ch<='f')n=ch-'a'+10;
        else return false;
        out=(out<<4)|n;
    }
    return true;
}
bool readEntry(const std::string& folder,const std::string& id,unsigned title,TrainerEntry& result) {
    const std::string manifest=join(folder,"trainer.axtrainer");
    if(!normal(folder,true)||!normal(manifest,false))return false;
    std::ifstream input(manifest.c_str(),std::ios::binary);
    if(!input)return false;
    char raw[4097];input.read(raw,sizeof(raw));
    if(input.bad()||input.gcount()>4096)return false;
    std::istringstream lines(std::string(raw,static_cast<size_t>(input.gcount())));
    std::map<std::string,std::string> fields;
    std::string line;
    while(std::getline(lines,line)) {
        if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
        if(line.empty()||line[0]=='#')continue;
        if(line.size()>256)return false;
        const size_t eq=line.find('=');
        if(eq==std::string::npos||fields.count(line.substr(0,eq)))return false;
        fields[line.substr(0,eq)]=line.substr(eq+1);
    }
    const bool schema2=fields["schema"]=="2";
    if((schema2?fields.size()!=8:fields.size()!=7)||(!schema2&&fields["schema"]!="1")||fields["mode"]!="standalone"||
       !hex32(fields["title_id"],result.titleId)||result.titleId!=title||
       !hex32(fields["media_id"],result.mediaId)||!result.mediaId||
       !hex32(fields["version"],result.version)||!result.version)return false;
    result.tuRequirement=TrainerEntry::Legacy;result.tuVersion=0;
    if(schema2) {
        const std::string requirement=fields["tu"];
        if(requirement=="none")result.tuRequirement=TrainerEntry::NoUpdate;
        else if(requirement=="unnumbered")result.tuRequirement=TrainerEntry::UnnumberedUpdate;
        else if(hex32(requirement,result.tuVersion)&&result.tuVersion)
            result.tuRequirement=TrainerEntry::ExactUpdate;
        else return false;
    }
    result.name=fields["name"];result.id=id;
    const std::string entry=fields["entry"];
    if(result.name.empty()||result.name.size()>80||!safe(entry)||
       entry.size()<5||entry.substr(entry.size()-4)!=".xex")return false;
    for(size_t i=0;i<result.name.size();++i)
        if(static_cast<unsigned char>(result.name[i])<32||result.name[i]==127)return false;
    result.launcher=join(folder,entry);
    if(!normal(result.launcher,false))return false;
    std::ifstream file(result.launcher.c_str(),std::ios::binary);
    char magic[4];
    return file.read(magic,4) && std::string(magic,4)=="XEX2";
}
}
TrainerEntry::Match TrainerEntry::Compatibility(const XboxExecutionInfo& game) const {
    if(!game.titleId||game.titleId!=titleId||!game.mediaId||!game.version)return Unknown;
    if(game.mediaId!=mediaId)return DifferentMedia;
    if(game.version!=version)return DifferentVersion;
    return HeaderMatches;
}
TrainerEntry::TuCheck TrainerEntry::CheckUpdates(const std::vector<XboxContentPackage>& packages) const {
    if(tuRequirement==Legacy)return LegacyHeaderOnly;
    if(tuRequirement==NoUpdate)return ActiveUnknown;
    size_t matches=0;
    for(size_t i=0;i<packages.size();++i) {
        const XboxContentPackage& p=packages[i];
        if(p.contentType!=0x000b0000u||p.titleId!=titleId||p.mediaId!=mediaId)continue;
        if(tuRequirement==UnnumberedUpdate || p.version==tuVersion)++matches;
    }
    if(!matches)return RequiredMissing;
    if(matches>1)return RequiredAmbiguous;
    return tuRequirement==UnnumberedUpdate?UnnumberedLocal:RequiredLocal;
}
std::vector<TrainerEntry> TrainerCatalog::Scan(const std::string& app,unsigned titleId) {
    std::vector<TrainerEntry> result;
    if(!titleId)return result;
    char id[9];std::sprintf(id,"%08X",titleId);
    std::vector<std::string> roots{join(app,"Trainers")};
    std::string base=app;
    while(!base.empty()&&(base.back()=='/'||base.back()=='\\'))base.pop_back();
    const auto sep=base.find_last_of("/\\");
    std::string leaf=base.substr(sep==std::string::npos?0:sep+1);
    for(char& ch:leaf)if(ch>='A'&&ch<='Z')ch=char(ch-'A'+'a');
    if(leaf=="menupreview")roots.push_back(join(sep==std::string::npos?"":base.substr(0,sep+1),"Trainers"));
    else roots.push_back(join(join(app,"MenuPreview"),"Trainers"));
    std::map<std::string,size_t> seen;std::set<std::string> conflicts;
    for(const auto& root:roots){
        const auto folder=join(root,id);
        if(!normal(root,true)||!normal(folder,true))continue;
        std::vector<FolderEntry> entries;if(!ReadFolder(folder,entries))continue;
        for(size_t i=0;i<entries.size()&&i<1024&&result.size()<64;++i){
            if(!entries[i].directory||!safe(entries[i].name))continue;
            TrainerEntry entry;
            if(!readEntry(join(folder,entries[i].name),entries[i].name,titleId,entry))continue;
            auto key=entry.id;for(char& ch:key)if(ch>='A'&&ch<='Z')ch=char(ch-'A'+'a');
            const auto old=seen.find(key);
            if(old!=seen.end()){
                if(result[old->second].launcher!=entry.launcher)conflicts.insert(key);
                continue;
            }
            seen[key]=result.size();result.push_back(entry);
        }
    }
    result.erase(std::remove_if(result.begin(),result.end(),[&](const TrainerEntry& entry){
        auto key=entry.id;for(char& ch:key)if(ch>='A'&&ch<='Z')ch=char(ch-'A'+'a');
        return conflicts.count(key)!=0;
    }),result.end());
    return result;
}
bool TrainerCatalog::Recheck(const std::string& app,const TrainerEntry& entry) {
    const std::vector<TrainerEntry> items=Scan(app,entry.titleId);
    for(size_t i=0;i<items.size();++i) {
        const TrainerEntry& item=items[i];
        if(item.id==entry.id&&item.launcher==entry.launcher&&item.name==entry.name&&
           item.mediaId==entry.mediaId&&item.version==entry.version&&
           item.tuRequirement==entry.tuRequirement&&item.tuVersion==entry.tuVersion)return true;
    }
    return false;
}
