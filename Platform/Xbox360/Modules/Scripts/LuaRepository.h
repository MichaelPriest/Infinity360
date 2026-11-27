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
struct Entry{std::string category,file,sha;size_t bytes=0;std::string url;};
inline bool RemoteUrl(const std::string& url){
    if(url.size()<12||url.size()>768||url.compare(0,8,"https://")!=0||url.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~:/?=&%+")!=std::string::npos)return false;
    const auto slash=url.find('/',8);return slash!=std::string::npos&&slash>8&&slash+1<url.size()&&url.find(':',8)>=slash;
}
inline std::string Join(const std::string& a,const std::string& b){
    return a+(a.empty()||a.back()=='/'||a.back()=='\\'?"":"/")+b;
}
inline bool Valid(const Entry& e){
    return (e.category=="Utilities"||e.category=="Filters"||e.category=="Sorts"||e.category=="Subtitles")
        && e.file.size()>4&&e.file.size()<=36&&e.file.substr(e.file.size()-4)==".lua"
        && e.file.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_- .")==std::string::npos
        && e.file.front()!='.'&&e.file.front()!=' '&&e.file.find("..")==std::string::npos
        && e.bytes>0&&e.bytes<=65536&&e.sha.size()==64&&e.sha.find_first_not_of("0123456789abcdef")==std::string::npos
        && (e.url.empty()||RemoteUrl(e.url));
}
inline std::string Source(const std::string& root,const Entry& e){return Join(Join(Join(root,"Scripts"),"Repository"),e.file);}
inline std::string Destination(const std::string& root,const Entry& e){
    const auto scripts=Join(root,"Scripts");return Join(e.category=="Utilities"?scripts:Join(scripts,e.category),e.file);
}
inline std::string Directory(const std::string& root){return Join(Join(root,"Scripts"),"Repository");}
inline bool ReadBounded(const std::string& path,size_t limit,std::string& data){
    std::ifstream in(path.c_str(),std::ios::binary|std::ios::ate);const auto n=in.tellg();
    if(n<0||n>static_cast<std::streamoff>(limit))return false;
    data.resize(static_cast<size_t>(n));in.seekg(0);return data.empty()||bool(in.read(&data[0],data.size()));
}
inline bool Parse(const std::string& data,std::vector<Entry>& out){
    out.clear();if(data.size()>32768||data.find('\0')!=std::string::npos)return false;
    std::istringstream in(data);std::string line;std::vector<Entry> rows;std::set<std::string> seen;
    auto strip=[](std::string& s){if(!s.empty()&&s.back()=='\r')s.pop_back();};
    if(!std::getline(in,line))return false;strip(line);
    const bool online=line=="INFINITY-LUA-REPO-V2";if(!online&&line!="INFINITY-LUA-REPO-V1")return false;
    while(std::getline(in,line)){
        strip(line);if(line.empty())continue;
        if(rows.size()>=128||line.size()>1024)return false;
        std::istringstream parts(line);Entry e;std::string count;
        if(!std::getline(parts,e.category,'\t')||!std::getline(parts,e.file,'\t')||!std::getline(parts,count,'\t'))return false;
        if(online){if(!std::getline(parts,e.sha,'\t'))return false;std::getline(parts,e.url);}
        else if(!std::getline(parts,e.sha))return false;
        if(count.empty()||count.size()>5||count.find_first_not_of("0123456789")!=std::string::npos)return false;
        for(char ch:count)e.bytes=e.bytes*10+size_t(ch-'0');
        if(!Valid(e))return false;
        auto key=e.file;for(auto& ch:key)if(ch>='A'&&ch<='Z')ch+=32;
        if(!seen.insert(key).second)return false;
        rows.push_back(e);
    }
    if(in.bad())return false;out.swap(rows);return true;
}
inline bool Load(const std::string& root,std::vector<Entry>& out){
    std::string data;out.clear();
    const auto cached=Join(Directory(root),"catalog-remote.tsv");
    if(ReadBounded(savedFile(cached),32768,data)&&Parse(data,out))return true;
    return ReadBounded(Join(Directory(root),"catalog.tsv"),32768,data)&&Parse(data,out);
}
struct Manifest{std::string url,sha;size_t bytes=0;};
inline bool ValidManifest(const Manifest& m){
    return RemoteUrl(m.url)&&m.bytes>0&&m.bytes<=32768&&m.sha.size()==64&&m.sha.find_first_not_of("0123456789abcdef")==std::string::npos;
}
inline bool ReadManifest(const std::string& root,Manifest& m){
    m={};std::string data;
    if(!ReadBounded(Join(Directory(root),"source.tsv"),1024,data)||data.find('\0')!=std::string::npos)return false;
    std::istringstream in(data);std::string header,line,extra,count;
    if(!std::getline(in,header)||!std::getline(in,line)||std::getline(in,extra))return false;
    if(!header.empty()&&header.back()=='\r')header.pop_back();
    if(!line.empty()&&line.back()=='\r')line.pop_back();
    if(header!="INFINITY-LUA-SOURCE-V1")return false;
    std::istringstream fields(line);
    if(!std::getline(fields,m.url,'\t')||!std::getline(fields,count,'\t')||!std::getline(fields,m.sha))return false;
    if(count.empty()||count.size()>5||count.find_first_not_of("0123456789")!=std::string::npos)return false;
    for(char ch:count)m.bytes=m.bytes*10+size_t(ch-'0');
    return ValidManifest(m);
}
inline std::string ImportManifest(const std::string& root,const Manifest& m,const std::string& data){
    if(!ValidManifest(m)||data.size()!=m.bytes)return "Manifesto Lua: tamanho invalido.";
    StoreSha256 hash;hash.Update(reinterpret_cast<const unsigned char*>(data.data()),data.size());
    if(hash.Finish()!=m.sha)return "Manifesto Lua: SHA-256 invalido.";
    std::vector<Entry> rows;
    if(!Parse(data,rows))return "Manifesto Lua: catalogo invalido.";
    const auto path=Join(Directory(root),"catalog-remote.tsv"),tmp=path+".tmp";
    if(fileExists(tmp))return "Temporario existente; catalogo anterior preservado.";
    std::ofstream out(tmp.c_str(),std::ios::binary|std::ios::trunc);
    out.write(data.data(),data.size());out.close();
    if(!out||!replaceSavedFile(path)){std::remove(tmp.c_str());return "Falha ao salvar catalogo remoto.";}
    return "Catalogo Lua sincronizado.";
}
inline bool Verify(const Entry& e,const std::string& data){
    if(!Valid(e)||data.size()!=e.bytes||data.find('\0')!=std::string::npos||static_cast<unsigned char>(data[0])==27)return false;
    StoreSha256 hash;hash.Update(reinterpret_cast<const unsigned char*>(data.data()),data.size());return hash.Finish()==e.sha;
}
inline bool ReadVerified(const std::string& path,const Entry& e,std::string& data){
    if(!Valid(e))return false;
    std::ifstream in(path.c_str(),std::ios::binary|std::ios::ate);
    if(in.tellg()!=static_cast<std::streamoff>(e.bytes))return false;
    data.resize(e.bytes);in.seekg(0);if(!in.read(&data[0],e.bytes))return false;
    return Verify(e,data);
}
inline std::string Status(const std::string& root,const Entry& e){
    if(!Valid(e))return "Invalido";
    std::string data;
    const auto dest=Destination(root,e);
    if(fileExists(dest))return ReadVerified(dest,e,data)?"Instalado":"Local diferente";
    return ReadVerified(Source(root,e),e,data)?"Disponivel":!e.url.empty()?"Online":"Fonte invalida";
}
inline std::string InstallData(const std::string& root,const Entry& e,const std::string& data){
    if(!Verify(e,data))return "Fonte ausente ou SHA-256 invalido.";
    const auto dest=Destination(root,e),tmp=dest+".tmp";
    if(fileExists(dest))return "Destino existente; arquivo preservado.";
    if(fileExists(tmp))return "Temporario existente; arquivo preservado.";
    std::ofstream out(tmp.c_str(),std::ios::binary|std::ios::trunc);out.write(data.data(),data.size());out.close();
    if(!out||fileExists(dest)||std::rename(tmp.c_str(),dest.c_str())){std::remove(tmp.c_str());return "Falha ao instalar script.";}
    return "Script instalado.";
}
inline std::string Install(const std::string& root,const Entry& e){
    if(!Valid(e))return "Entrada invalida.";
    std::string data;
    if(!ReadVerified(Source(root,e),e,data))return "Fonte ausente ou SHA-256 invalido.";
    return InstallData(root,e,data);
}
inline std::string Remove(const std::string& root,const Entry& e){
    if(!Valid(e))return "Entrada invalida.";
    std::string data;const auto dest=Destination(root,e);
    if(!ReadVerified(dest,e,data))return "Script ausente ou alterado; preservado.";
    return std::remove(dest.c_str())?"Falha ao remover script.":"Script removido.";
}
}
