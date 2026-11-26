#pragma once
#include <array>
#include <cstdlib>
#include <algorithm>
namespace ApprovedHome {
enum Slot { Home, Games, Apps, Iptv, Store, Downloads, System, Network, Profiles, Achievements, Settings, Power,
            Hero, Disc, Files, Link, Tv, Shop, Awards, FirstCover, End=FirstCover+10 };
struct Rect {int x,y,w,h;};
inline Rect Geometry(int i){
    if(i<Hero)return {28+i*103,83,98,74};
    if(i==Hero)return {28,177,596,226};
    if(i<FirstCover){int j=i-Disc;return {638+j%3*205,177+j/3*116,197,110};}
    return {28+(i-FirstCover)*123,448,116,174};
}
inline int Move(int current,int direction,int covers){
    if(current<0||current>=End||direction<0||direction>3)return Home;
    const int end=FirstCover+std::max(0,std::min(10,covers));
    if(current>=end)current=Home;
    const auto a=Geometry(current);int best=current;long score=0x7fffffff;
    for(int i=0;i<end;++i){
        if(i==current)continue;auto b=Geometry(i);
        int dx=2*b.x+b.w-2*a.x-a.w,dy=2*b.y+b.h-2*a.y-a.h;
        int forward=direction==0?-dx:direction==1?dx:direction==2?-dy:dy;
        if(forward<=0)continue;int lateral=direction<2?std::abs(dy):std::abs(dx);
        long candidate=long(forward)+4L*lateral;if(candidate<score){score=candidate;best=i;}
    }
    return best;
}
}
