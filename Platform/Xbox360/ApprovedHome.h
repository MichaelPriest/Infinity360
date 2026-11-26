#pragma once
#include <array>
#include <cstdlib>
#include <algorithm>
namespace ApprovedHome {
enum Slot { Home, Games, Apps, Iptv, Store, Downloads, System, Network, Profiles, Achievements, Settings, Power,
            Hero, Disc, Files, Link, Tv, Shop, Awards, FirstCover, Weather=FirstCover+6, End };
struct Rect {int x,y,w,h;};
inline Rect Geometry(int i){
    if(i==Weather)return {772,12,266,52};
    if(i<Hero){
        static const int widths[]={94,88,75,79,74,114,94,74,84,111,107,86};
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
    if(current>=end&&current!=Weather)current=Home;
    const auto a=Geometry(current);int best=current;long score=0x7fffffff;
    for(int i=0;i<End;++i){
        if(i==current||(i>=end&&i!=Weather))continue;auto b=Geometry(i);
        int dx=2*b.x+b.w-2*a.x-a.w,dy=2*b.y+b.h-2*a.y-a.h;
        int forward=direction==0?-dx:direction==1?dx:direction==2?-dy:dy;
        if(forward<=0)continue;int lateral=direction<2?std::abs(dy):std::abs(dx);
        long candidate=long(forward)+4L*lateral;if(candidate<score){score=candidate;best=i;}
    }
    return best;
}
}
