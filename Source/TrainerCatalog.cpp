#include "TrainerCatalog.h"
#include "GameScanner.h"
#include <algorithm>
#include <cstdio>
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
    if(fields.size()!=7||fields["schema"]!="1"||fields["mode"]!="standalone"||
       !hex32(fields["title_id"],result.titleId)||result.titleId!=title||
       !hex32(fields["media_id"],result.mediaId)||!result.mediaId||
       !hex32(fields["version"],result.version)||!result.version)return false;
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
std::vector<TrainerEntry> TrainerCatalog::Scan(const std::string& app,unsigned titleId) {
    std::vector<TrainerEntry> result;
    if(!titleId)return result;
    char id[9];std::sprintf(id,"%08X",titleId);
    const std::string root=join(app,"Trainers"), folder=join(root,id);
    if(!normal(root,true)||!normal(folder,true))return result;
    std::vector<FolderEntry> entries;
    if(!ReadFolder(folder,entries))return result;
    for(size_t i=0;i<entries.size()&&i<1024&&result.size()<64;++i) {
        if(!entries[i].directory||!safe(entries[i].name))continue;
        TrainerEntry entry;
        if(readEntry(join(folder,entries[i].name),entries[i].name,titleId,entry))
            result.push_back(entry);
    }
    return result;
}
bool TrainerCatalog::Recheck(const std::string& app,const TrainerEntry& entry) {
    const std::vector<TrainerEntry> items=Scan(app,entry.titleId);
    for(size_t i=0;i<items.size();++i) {
        const TrainerEntry& item=items[i];
        if(item.id==entry.id&&item.launcher==entry.launcher&&item.name==entry.name&&
           item.mediaId==entry.mediaId&&item.version==entry.version)return true;
    }
    return false;
}
