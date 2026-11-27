#pragma once
#include <string>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <vector>
#include "HdArtwork.h"
namespace TvRemote {
inline std::string LowerAscii(std::string s){for(char& c:s)if(c>='A'&&c<='Z')c+=32;return s;}
inline bool Matches(const std::string& text,const std::string& query){return query.empty()||LowerAscii(text).find(LowerAscii(query))!=std::string::npos;}
inline std::string Quote(const std::string& input,size_t limit=48){
    std::string out="\"";
    for(size_t i=0;i<input.size()&&i<limit;){
        unsigned char c=input[i];if(c<128){++i;if(c<32||c==127){out+=' ';continue;}if(c=='"'||c=='\\')out+='\\';out+=char(c);continue;}
        size_t n=c>=0xc2&&c<=0xdf?2:c>=0xe0&&c<=0xef?3:c>=0xf0&&c<=0xf4?4:0;
        if(n&&i+n>limit)break;
        bool good=n&&i+n<=input.size();
        if(good){for(size_t k=1;k<n;++k)if((static_cast<unsigned char>(input[i+k])&0xc0)!=0x80)good=false;
            const unsigned char d=input[i+1];if((c==0xe0&&d<0xa0)||(c==0xed&&d>=0xa0)||(c==0xf0&&d<0x90)||(c==0xf4&&d>=0x90))good=false;}
        if(!good){out+='?';++i;}else{out.append(input,i,n);i+=n;}
    }
    return out+"\"";
}
inline std::string GameArtId(const std::string& path){
    uint64_t hash=14695981039346656037ull;
    for(unsigned char c:path){hash^=c;hash*=1099511628211ull;}
    return std::to_string(hash?hash:1);
}
inline std::string CoverThumbnail(const std::string& path){
    if(path.empty())return "{\"available\":false}";
    std::vector<unsigned char> data;HdArtwork::Size size{};
    if(!HdArtwork::Read(path,true,data,size))return "{\"available\":false}";
    const char* hex="0123456789abcdef";
    std::string out="{\"available\":true,\"w\":20,\"h\":30,\"rgba\":\"";
    for(unsigned y=0;y<30;++y)for(unsigned x=0;x<20;++x){
        const size_t offset=((y*size.height/30+size.height/60)*size.width+x*size.width/20+size.width/40)*4;
        for(unsigned k=0;k<4;++k){const auto value=data[offset+k];out+=hex[value>>4];out+=hex[value&15];}
    }
    return out+"\"}";
}
template<class Row> std::string CatalogPage(size_t total,unsigned offset,Row row){
    size_t i=std::min<size_t>(offset,total),count=0,scanned=0;
    std::string out="{\"schema\":1,\"items\":[";
    for(;i<total&&count<16&&scanned<1024;++i,++scanned){
        const std::string item=row(i);if(item.empty())continue;
        if(count++)out+=",";
        out+=item;
    }
    out+="],\"next\":"+(i<total?std::to_string(i):std::string("null"))+"}";
    return out;
}
}
