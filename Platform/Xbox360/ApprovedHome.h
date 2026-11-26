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
inline size_t PageIndex(size_t page,size_t count){return std::min(page,Pages(count)-1);}
inline size_t Visible(size_t page,size_t count){return std::min(ShelfSize,count-PageIndex(page,count)*ShelfSize);}
inline size_t Featured(size_t index,size_t count){return count?index%std::min(HeroSize,count):0;}
inline bool ValidOrder(const std::string& order){
    if(order.size()!=6)return false;
    unsigned mask=0;for(char ch:order){if(ch<'0'||ch>'5'||(mask&(1u<<(ch-'0'))))return false;mask|=1u<<(ch-'0');}return true;
}
inline bool CoverSlot(int slot){return slot>=FirstCover&&slot<Weather;}
inline bool ExtraSlot(int slot){return slot==Weather||slot==LibraryLink;}
// Keep stable action IDs; only these slots are navigable on the compact Home.
inline bool NavSlot(int i){return i>=Home&&i<=Store||i==System;}
inline bool GroupSlot(int i){return i==Disc||i==Link||i==Awards;}
inline bool HiddenSlot(int i){return (i<Hero&&!NavSlot(i))||(i>Hero&&i<FirstCover&&!GroupSlot(i));}
inline int GroupPosition(int i){return i==Disc?0:i==Link?1:2;}
inline std::array<int,3> GroupOrder(const std::string& order){
    std::array<int,3> result{{0,1,2}};if(!ValidOrder(order))return result;
    bool seen[3]={false,false,false};int n=0;
    for(char ch:order){int group=(ch-'0')/2;if(!seen[group]){seen[group]=true;result[n++]=group;}}return result;
}
inline std::array<int,4> GroupActions(int group,const std::string& order){
    if(group==3)return {{System,Settings,Power,Downloads}};
    const int fixed[3][2]={{Disc,Files},{Link,Tv},{Shop,Awards}};
    const int extras[3]={Downloads,Network,Profiles};
    if(group<0||group>=3)return {{-1,-1,-1,-1}};
    std::array<int,4> result{{fixed[group][0],fixed[group][1],extras[group],-1}};
    if(ValidOrder(order)&&order.find(char('0'+group*2))>order.find(char('1'+group*2)))std::swap(result[0],result[1]);
    return result;
}
inline Rect Geometry(int i){
    if(i==Weather)return {772,12,266,52};
    if(i==LibraryLink)return {1070,400,182,37};
    if(i<Hero){
        const int n=i==System?5:i;
        return {28+n*205,94,195,46};
    }
    if(i==Hero)return {28,158,690,230};
    if(i<FirstCover){return {730+GroupPosition(i)*177,158,166,230};}
    return {28+(i-FirstCover)*205,450,195,175};
}
inline int Move(int current,int direction,int covers){
    if(current<0||current>=End||direction<0||direction>3)return Home;
    const int end=FirstCover+std::max(0,std::min(6,covers));
    if((current>=end&&!ExtraSlot(current))||HiddenSlot(current))current=Home;
    const auto a=Geometry(current);int best=current;long score=0x7fffffff;
    for(int i=0;i<End;++i){
        if(i==current||(i>=end&&!ExtraSlot(i))||HiddenSlot(i))continue;auto b=Geometry(i);
        int dx=2*b.x+b.w-2*a.x-a.w,dy=2*b.y+b.h-2*a.y-a.h;
        int forward=direction==0?-dx:direction==1?dx:direction==2?-dy:dy;
        if(forward<=0)continue;int lateral=direction<2?std::abs(dy):std::abs(dx);
        long candidate=long(forward)+4L*lateral;if(candidate<score){score=candidate;best=i;}
    }
    return best;
}
}
