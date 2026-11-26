#pragma once
#include <string>
#include <fstream>
#include <set>
#include <cstdio>
namespace InfinityFps {
// Read-only bounded index of the line-oriented Nexus XML format.
// No general XML parser, entity expansion, network, patch writes or execution.
inline size_t CountFor(const std::string& root,unsigned title){
    if(!title)return 0;
    std::ifstream in((root+"Patches/FpsPatchCatalog.xml").c_str(),std::ios::binary);
    if(!in)return 0;
    char key[9];std::sprintf(key,"%08X",title);
    std::set<std::string> names;
    char raw[2049];size_t bytes=0;
    while(in.getline(raw,sizeof raw)){
        bytes+=static_cast<size_t>(in.gcount());if(bytes>512*1024)return 0;
        std::string line(raw);const auto start=line.find_first_not_of(" \t");
        if(start==std::string::npos||line.compare(start,12,"<file name=\"")!=0)continue;
        const auto end=line.find('"',start+12);if(end==std::string::npos)continue;
        auto name=line.substr(start+12,end-start-12);
        if(name.size()<23||name.size()>240||name.find_first_of("/\\")!=std::string::npos)continue;
        auto prefix=name.substr(0,8);
        for(char& ch:prefix)if(ch>='a'&&ch<='f')ch=char(ch-'a'+'A');
        if(prefix!=key||name.compare(8,3," - ")!=0||name.compare(name.size()-11,11,".patch.toml")!=0)continue;
        names.insert(name);if(names.size()>=512)break;
    }
    if(in.bad()||(!in.eof()&&in.fail()))return 0;
    return names.size();
}
}
