#pragma once
#include <string>
#include <cstdint>
namespace DownloadRange {
inline bool ParseNumber(const std::string& s,size_t& pos,uint64_t& value){
    value=0;size_t start=pos;
    while(pos<s.size()&&s[pos]>='0'&&s[pos]<='9'){
        unsigned n=static_cast<unsigned>(s[pos++]-'0');
        if(value>(UINT64_MAX-n)/10)return false;
        value=value*10+n;
    }
    return pos>start;
}
inline bool Matches(const std::string& s,uint64_t offset,uint64_t total){
    if(!offset||offset>=total||s.compare(0,6,"bytes ")!=0)return false;
    size_t p=6;uint64_t first,last,size;
    return ParseNumber(s,p,first)&&p<s.size()&&s[p++]=='-'&&
      ParseNumber(s,p,last)&&p<s.size()&&s[p++]=='/'&&
      ParseNumber(s,p,size)&&p==s.size()&&first==offset&&last==total-1&&size==total;
}
}
