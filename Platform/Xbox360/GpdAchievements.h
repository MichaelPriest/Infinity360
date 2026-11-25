#pragma once
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <set>
#include <string>
#include <vector>

// Read-only parser for explicitly exported big-endian Xbox GPD files.
// Reference: Free60 GPD and XDBF format documents. No STFS extraction or edits.
namespace GpdAchievements {
struct Title {uint32_t id=0,total=0,earned=0,score=0,earnedScore=0;std::string name;};
struct Achievement {uint32_t id=0,score=0;bool earned=false,online=false;std::string name;};
struct Summary {std::vector<Title> titles;uint64_t total=0,earned=0,score=0,earnedScore=0;};
inline uint16_t U16(const uint8_t* p){return (uint16_t(p[0])<<8)|p[1];}
inline uint32_t U32(const uint8_t* p){return (uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];}
inline uint64_t U64(const uint8_t* p){return (uint64_t(U32(p))<<32)|U32(p+4);}
inline bool Text(const uint8_t* p,size_t n,std::string& out){
    out.clear();
    for(size_t i=0;i+1<n;i+=2){
        uint32_t cp=U16(p+i);
        if(!cp)return true;
        if(cp>=0xd800&&cp<=0xdbff){
            if(i+3>=n)return false;
            const auto lo=U16(p+i+2);if(lo<0xdc00||lo>0xdfff)return false;
            cp=0x10000+((cp-0xd800)<<10)+(lo-0xdc00);i+=2;
        }else if(cp>=0xdc00&&cp<=0xdfff)return false;
        if(cp<32||cp==127||(cp>=0x202a&&cp<=0x202e)||(cp>=0x2066&&cp<=0x2069))continue;
        if(out.size()>240)continue;
        if(cp<128)out+=char(cp);
        else if(cp<2048){out+=char(0xc0|(cp>>6));out+=char(0x80|(cp&63));}
        else if(cp<65536){out+=char(0xe0|(cp>>12));out+=char(0x80|((cp>>6)&63));out+=char(0x80|(cp&63));}
        else{out+=char(0xf0|(cp>>18));out+=char(0x80|((cp>>12)&63));out+=char(0x80|((cp>>6)&63));out+=char(0x80|(cp&63));}
    }
    return false;
}
struct Entry {uint16_t kind;uint64_t id;size_t offset,length;};
inline bool Index(const std::vector<uint8_t>& data,std::vector<Entry>& entries){
    entries.clear();
    if(data.size()<24||data.size()>16*1024*1024||std::memcmp(data.data(),"XDBF",4)||U32(data.data()+4)!=0x10000)return false;
    const auto cap=U32(data.data()+8),count=U32(data.data()+12),freeCap=U32(data.data()+16),freeCount=U32(data.data()+20);
    if(cap>65536||freeCap>65536||count>cap||freeCount>freeCap)return false;
    const uint64_t base=24ULL+18ULL*cap+8ULL*freeCap;
    if(base>data.size())return false;
    std::set<std::pair<uint16_t,uint64_t>> keys;
    std::vector<std::pair<size_t,size_t>> ranges;
    for(uint32_t i=0;i<count;++i){
        const auto* p=data.data()+24+size_t(i)*18;
        const auto kind=U16(p);const auto id=U64(p+2);const uint64_t start=base+U32(p+10),len=U32(p+14);
        if(start>data.size()||len>data.size()-start||!keys.insert({kind,id}).second)return false;
        if(len)ranges.push_back({size_t(start),size_t(start+len)});
        entries.push_back({kind,id,size_t(start),size_t(len)});
    }
    std::sort(ranges.begin(),ranges.end());
    for(size_t i=1;i<ranges.size();++i)if(ranges[i].first<ranges[i-1].second)return false;
    return true;
}
inline bool Dashboard(const std::vector<uint8_t>& data,Summary& out){
    out={};std::vector<Entry> entries;if(!Index(data,entries))return false;
    Summary next;
    for(const auto& e:entries){
        if(e.kind!=4||e.id==0x100000000ULL||e.id==0x200000000ULL)continue;
        if(!e.id||e.id>0xffffffffULL||e.length<42||next.titles.size()>=4096)return false;
        const auto* p=data.data()+e.offset;Title t;
        t.id=U32(p);t.total=U32(p+4);t.earned=U32(p+8);t.score=U32(p+12);t.earnedScore=U32(p+16);
        if(t.id!=e.id||t.total>100000||t.earned>t.total||t.score>1000000||t.earnedScore>t.score||!Text(p+40,e.length-40,t.name))return false;
        next.total+=t.total;next.earned+=t.earned;next.score+=t.score;next.earnedScore+=t.earnedScore;
        next.titles.push_back(std::move(t));
    }
    std::sort(next.titles.begin(),next.titles.end(),[](const Title& a,const Title& b){return a.name==b.name?a.id<b.id:a.name<b.name;});
    out=std::move(next);return true;
}
inline bool Game(const std::vector<uint8_t>& data,std::vector<Achievement>& out){
    out.clear();std::vector<Entry> entries;if(!Index(data,entries))return false;
    std::vector<Achievement> next;
    for(const auto& e:entries){
        if(e.kind!=1||e.id==0x100000000ULL||e.id==0x200000000ULL)continue;
        if(!e.id||e.id>0xffffffffULL||e.length<30||next.size()>=4096)return false;
        const auto* p=data.data()+e.offset;Achievement a;
        a.id=U32(p+4);a.score=U32(p+12);const auto flags=U32(p+16);
        if(U32(p)!=28||a.id!=e.id||a.score>1000000)return false;
        a.online=(flags&0x10000)!=0;a.earned=(flags&0x20000)!=0||a.online;
        if(!Text(p+28,e.length-28,a.name))return false;
        // Preserve secrecy for locked hidden achievements.
        if(!a.earned&&!(flags&8))a.name="Conquista secreta";
        next.push_back(std::move(a));
    }
    std::sort(next.begin(),next.end(),[](const Achievement& a,const Achievement& b){return a.id<b.id;});
    out=std::move(next);return true;
}
inline bool Read(const std::string& path,std::vector<uint8_t>& data){
    data.clear();std::ifstream in(path.c_str(),std::ios::binary|std::ios::ate);
    const auto n=in.tellg();if(!in||n<24||n>16*1024*1024)return false;
    data.resize(static_cast<size_t>(n));in.seekg(0);
    if(!in.read(reinterpret_cast<char*>(data.data()),static_cast<std::streamsize>(data.size()))||
        in.peek()!=std::char_traits<char>::eof()){data.clear();return false;}
    return true;
}
}
