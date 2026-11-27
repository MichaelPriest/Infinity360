#pragma once
#include <string>
#include <cstdint>
namespace CloudRange {
constexpr uint64_t MaxLength=8ULL*1024*1024*1024;
inline bool Number(const std::string& s,uint64_t& n){
    n=0;if(s.empty())return false;
    for(char c:s){if(c<'0'||c>'9'||n>(MaxLength-unsigned(c-'0'))/10)return false;n=n*10+unsigned(c-'0');}
    return true;
}
struct Range{uint64_t first=0,last=0,total=0;};
inline bool Parse(const std::string& s,Range& out){
    if(s.compare(0,6,"bytes ")!=0)return false;
    auto dash=s.find('-',6),slash=s.find('/',6);
    Range r;
    if(dash==std::string::npos||slash==std::string::npos||dash>=slash||
       !Number(s.substr(6,dash-6),r.first)||!Number(s.substr(dash+1,slash-dash-1),r.last)||
       !Number(s.substr(slash+1),r.total)||r.first>r.last||r.last>=r.total)return false;
    out=r;return true;
}
inline bool StrongTag(const std::string& s){
    if(s.size()<2||s.size()>256||s.front()!='"'||s.back()!='"')return false;
    for(size_t i=1;i+1<s.size();++i){auto c=static_cast<unsigned char>(s[i]);if(c<33||c==34||c==127)return false;}
    return true;
}
inline bool Matches(const Range& r,uint64_t first,uint64_t last,uint64_t known){
    return r.total>0&&r.total<=MaxLength&&r.first==first&&first<r.total&&
       r.last==(last<r.total?last:r.total-1)&&(!known||known==r.total);
}
}
