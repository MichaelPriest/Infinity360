#pragma once
#include <string>
#include <map>
#include <fstream>
namespace InfinityPs1 {
inline std::string Lower(std::string s){for(char& c:s)if(c>='A'&&c<='Z')c=char(c-'A'+'a');return s;}
inline bool Name(const std::string& s){
    if(s.empty()||s.size()>240||s=="."||s==".."||s.find_first_of("/\\:")!=std::string::npos)return false;
    for(unsigned char c:s)if(c<32||c==127)return false;
    return true;
}
struct Index{
    bool valid=false;
    std::map<std::string,std::string> names;
    std::string Find(const std::string& filename) const{
        if(!valid||!Name(filename))return "";
        const auto dot=filename.find_last_of('.');
        if(dot==std::string::npos||dot==0)return "";
        const auto it=names.find(Lower(filename.substr(0,dot)));
        return it==names.end()?"":it->second;
    }
};
inline Index Load(const std::string& root){
    Index out;
    std::ifstream in((root+"Andromeda/Ps1CoverIndex.txt").c_str(),std::ios::binary);
    char raw[1025];if(!in.getline(raw,sizeof raw))return out;
    std::string header(raw);if(!header.empty()&&header.back()=='\r')header.pop_back();
    if(header.compare(0,3,"\xef\xbb\xbf")==0)header.erase(0,3);
    if(header!="NEXUS_PS1_COVERS_V1")return out;
    size_t bytes=static_cast<size_t>(in.gcount());
    while(in.getline(raw,sizeof raw)){
        bytes+=static_cast<size_t>(in.gcount());if(bytes>2*1024*1024)return Index();
        std::string name(raw);if(!name.empty()&&name.back()=='\r')name.pop_back();
        if(!Name(name)||name.size()<5||Lower(name.substr(name.size()-4))!=".png")continue;
        out.names.emplace(Lower(name.substr(0,name.size()-4)),name);
        if(out.names.size()>12000)return Index();
    }
    if(in.bad()||(!in.eof()&&in.fail()))return Index();
    out.valid=true;return out;
}
}
