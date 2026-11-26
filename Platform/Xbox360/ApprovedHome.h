#pragma once
#include <array>
#include <cstdlib>
#include <algorithm>
#include <string>
namespace ApprovedHome {
enum Slot { Home, Games, Apps, Iptv, Store, Downloads, System, Network, Profiles, Achievements, Settings, Power,
            Hero, Disc, Files, Link, Tv, Shop, Awards, FirstCover, Weather=FirstCover+6, LibraryLink, End };
struct Rect {int x,y,w,h;};
constexpr size_t ShelfSize=6,HeroSize=4;
inline size_t Pages(size_t count){return count?1+(count-1)/ShelfSize:1;}
inline size_t Page(size_t page,size_t count){return std::min(page,Pages(count)-1);}
inline size_t Visible(size_t page,size_t count){return std::min(ShelfSize,count-Page(page,count)*ShelfSize);}
inline size_t Featured(size_t index,size_t count){return count?index%std::min(HeroSize,count):0;}
inline bool ValidOrder(const std::string& order){
    if(order.size()!=6)return false;
    unsigned mask=0;for(char ch:order){if(ch<'0'||ch>'5'||(mask&(1u<<(ch-'0'))))return false;mask|=1u<<(ch-'0');}return true;
}
inline bool CoverSlot(int slot){return slot>=FirstCover&&slot<Weather;}
inline bool ExtraSlot(int slot){return slot==Weather||slot==LibraryLink;}
inline Rect Geometry(int i){
    if(i==Weather)return {772,12,266,52};
    if(i==LibraryLink)return {1070,400,182,37};
    if(i<Hero){
        static const int widths[]={99,86,75,78,74,112,100,78,87,120,119,90};
        int x=28;for(int j=0;j<i;++j)x+=widths[j]+8;
        return {x,94,widths[i],46};
    }
    if(i==Hero)return {28,158,690,230};
    if(i<FirstCover){int j=i-Disc;return {730+j%3*177,158+j/3*119,166,110};}
    return {28+(i-FirstCover)*205,450,195,175};
}
inline int Move(int current,int direction,int covers){
    if(current<0||current>=End||direction<0||direction>3)return Home;
    const int end=FirstCover+std::max(0,std::min(6,covers));
    if(current>=end&&!ExtraSlot(current))current=Home;
    const auto a=Geometry(current);int best=current;long score=0x7fffffff;
    for(int i=0;i<End;++i){
        if(i==current||(i>=end&&!ExtraSlot(i)))continue;auto b=Geometry(i);
        int dx=2*b.x+b.w-2*a.x-a.w,dy=2*b.y+b.h-2*a.y-a.h;
        int forward=direction==0?-dx:direction==1?dx:direction==2?-dy:dy;
        if(forward<=0)continue;int lateral=direction<2?std::abs(dy):std::abs(dx);
        long candidate=long(forward)+4L*lateral;if(candidate<score){score=candidate;best=i;}
    }
    return best;
}
}
