#pragma once
#include "StoreCatalog.h"
#include "FileWorkspace.h"
#include <fstream>
#include <sstream>
namespace DashboardUpdate {
inline bool ValidId(const std::string& id){
    return id.size()==21&&id.compare(0,9,"infinity-")==0&&id.substr(9).find_first_not_of("0123456789abcdef")==std::string::npos;
}
inline bool Parse(const std::string& data,StoreItem& out){
    out={};if(data.empty()||data.size()>256||data.find('\0')!=std::string::npos)return false;
    std::istringstream in(data);std::string line,extra;
    if(!std::getline(in,line)||std::getline(in,extra))return false;
    if(!line.empty()&&line.back()=='\r')line.pop_back();
    if(line.empty()||line.back()=='\t')return false;
    std::istringstream fields(line);std::string id,version,bytes,digest;
    if(!std::getline(fields,id,'\t')||!std::getline(fields,version,'\t')||!std::getline(fields,bytes,'\t')||!std::getline(fields,digest,'\t')||std::getline(fields,extra,'\t'))return false;
    if(!ValidId(id)||version.empty()||version.size()>16||version.find_first_not_of("0123456789.")!=std::string::npos||
       bytes.empty()||bytes.size()>9||bytes.find_first_not_of("0123456789")!=std::string::npos||
       digest.size()!=64||digest.find_first_not_of("0123456789abcdef")!=std::string::npos)return false;
    unsigned long long size=0;for(char c:bytes)size=size*10+c-'0';
    if(size<4096||size>64ULL*1024*1024)return false;
    out.id=id;out.name="INFINITY 360 "+version;out.version=version;out.category="dashboard";out.sha256=digest;
    out.bytes=size;out.rights="Manually approved INFINITY 360 release";return true;
}
inline bool Read(const std::string& root,StoreItem& out){
    out={};FileWorkspace ws(root);
    const auto path=FileWorkspace::Join(root,"infinity360-update.tsv");
    if(!ws.CanReadFile(path))return false;
    std::ifstream input(path.c_str(),std::ios::binary|std::ios::ate);
    if(!input||input.tellg()<1||input.tellg()>256)return false;
    std::string data(static_cast<size_t>(input.tellg()),'\0');input.seekg(0);
    return bool(input.read(&data[0],data.size()))&&Parse(data,out);
}
inline bool Enabled(const std::string& root){
    std::ifstream flag(FileWorkspace::Join(root,"enable-dashboard-update.txt").c_str());
    std::string line;return bool(std::getline(flag,line))&&line=="ENABLE-DASHBOARD-UPDATE";
}
}
