#pragma once
#include "../../StoreSha256.h"
#include "../../SavedFile.h"
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <set>
#include <cstdio>
namespace InfinityLuaRepo {
struct Entry{std::string category,file,sha;size_t bytes=0;};
inline std::string Join(const std::string& a,const std::string& b){
    return a+(a.empty()||a.back()=='/'||a.back()=='\\'?"":"/")+b;
}
inline bool Valid(const Entry& e){
    return (e.category=="Utilities"||e.category=="Filters"||e.category=="Sorts"||e.category=="Subtitles")
        && e.file.size()>4&&e.file.size()<=36&&e.file.substr(e.file.size()-4)==".lua"
        && e.file.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_- .")==std::string::npos
        && e.file.front()!='.'&&e.file.front()!=' '&&e.file.find("..")==std::string::npos
        && e.bytes>0&&e.bytes<=65536&&e.sha.size()==64&&e.sha.find_first_not_of("0123456789abcdef")==std::string::npos;
}
inline std::string Source(const std::string& root,const Entry& e){return Join(Join(Join(root,"Scripts"),"Repository"),e.file);}
inline std::string Destination(const std::string& root,const Entry& e){
    const auto scripts=Join(root,"Scripts");return Join(e.category=="Utilities"?scripts:Join(scripts,e.category),e.file);
}
inline bool Load(const std::string& root,std::vector<Entry>& out){
    out.clear();std::ifstream in(Join(Join(Join(root,"Scripts"),"Repository"),"catalog.tsv"),std::ios::binary|std::ios::ate);
    const auto length=in.tellg();if(length<0||length>32768)return false;
    in.seekg(0);std::string line;std::vector<Entry> rows;std::set<std::string> seen;
    auto strip=[](std::string& s){if(!s.empty()&&s.back()=='\r')s.pop_back();};
    if(!std::getline(in,line))return false;strip(line);if(line!="INFINITY-LUA-REPO-V1")return false;
    while(std::getline(in,line)){
        strip(line);if(line.empty())continue;
        if(rows.size()>=128||line.size()>256)return false;
        std::istringstream parts(line);Entry e;std::string count;
        if(!std::getline(parts,e.category,'\t')||!std::getline(parts,e.file,'\t')||!std::getline(parts,count,'\t')||!std::getline(parts,e.sha))return false;
        if(count.empty()||count.size()>5||count.find_first_not_of("0123456789")!=std::string::npos)return false;
        for(char ch:count)e.bytes=e.bytes*10+size_t(ch-'0');
        if(!Valid(e))return false;
        auto key=e.file;for(auto& ch:key)if(ch>='A'&&ch<='Z')ch+=32;
        if(!seen.insert(key).second)return false;
        rows.push_back(e);
    }
    if(in.bad())return false;out.swap(rows);return true;
}
inline bool ReadVerified(const std::string& path,const Entry& e,std::string& data){
    if(!Valid(e))return false;
    std::ifstream in(path.c_str(),std::ios::binary|std::ios::ate);
    if(in.tellg()!=static_cast<std::streamoff>(e.bytes))return false;
    data.resize(e.bytes);in.seekg(0);if(!in.read(&data[0],e.bytes))return false;
    if(data.find('\0')!=std::string::npos||static_cast<unsigned char>(data[0])==27)return false;
    StoreSha256 hash;hash.Update(reinterpret_cast<const unsigned char*>(data.data()),data.size());
    return hash.Finish()==e.sha;
}
inline std::string Status(const std::string& root,const Entry& e){
    if(!Valid(e))return "Invalido";
    std::string data;
    const auto dest=Destination(root,e);
    if(fileExists(dest))return ReadVerified(dest,e,data)?"Instalado":"Local diferente";
    return ReadVerified(Source(root,e),e,data)?"Disponivel":"Fonte invalida";
}
inline std::string Install(const std::string& root,const Entry& e){
    if(!Valid(e))return "Entrada invalida.";
    std::string data;
    if(!ReadVerified(Source(root,e),e,data))return "Fonte ausente ou SHA-256 invalido.";
    const auto dest=Destination(root,e),tmp=dest+".tmp";
    if(fileExists(dest))return "Destino existente; arquivo preservado.";
    if(fileExists(tmp))return "Temporario existente; arquivo preservado.";
    std::ofstream out(tmp.c_str(),std::ios::binary|std::ios::trunc);out.write(data.data(),data.size());out.close();
    if(!out||fileExists(dest)||std::rename(tmp.c_str(),dest.c_str())){std::remove(tmp.c_str());return "Falha ao instalar script.";}
    return "Script instalado.";
}
inline std::string Remove(const std::string& root,const Entry& e){
    if(!Valid(e))return "Entrada invalida.";
    std::string data;const auto dest=Destination(root,e);
    if(!ReadVerified(dest,e,data))return "Script ausente ou alterado; preservado.";
    return std::remove(dest.c_str())?"Falha ao remover script.":"Script removido.";
}
}
