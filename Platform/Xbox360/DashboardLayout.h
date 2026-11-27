#pragma once
#include <array>
#include <string>
#include <cstdlib>

// Home navigation uses the same rectangles for rendering and focus movement.
// Classic is unchanged; the nine modern layouts share existing destinations.
namespace DashboardLayout {
enum Style { Classic, TopIcons, LateralPro, Hybrid, BladeNeo, Immersive, Minimal, Arcade, MediaHub, Performance, Count };
constexpr int Slots=10,DiscSlot=Slots;
struct Rect {int x,y,w,h;};
struct Geometry {std::array<Rect,Slots> nav{};Rect hero{},shelf{};};
inline const char* Name(unsigned i){
    static const char* names[]={"Classic (legado)","Series Infinity","Lateral Pro","Hybrid","Blade Neo","Immersive","Minimal","Arcade","Media Hub","Performance"};
    return names[i<Count?i:0];
}
inline bool Parse(const std::string& s,unsigned& out){
    if(s.size()!=1||s[0]<'0'||s[0]>'9')return false;
    out=unsigned(s[0]-'0');return true;
}
inline Geometry Get(unsigned style){
    Geometry g;
    if(style==LateralPro||style==MediaHub){
        for(int i=0;i<Slots;++i)g.nav[i]={44,83+i*53,230,49};
        g.hero={294,83,940,301};g.shelf={294,397,940,222};
        if(style==MediaHub){g.hero={294,83,940,220};g.shelf={294,316,940,303};}
    }else if(style==Minimal||style==Performance){
        for(int i=0;i<Slots;++i)g.nav[i]={44+(i%5)*240,125+(i/5)*109,230,99};
        g.hero={44,351,584,268};g.shelf={642,351,592,268};
        if(style==Performance){
            for(int i=0;i<Slots;++i)g.nav[i]={44+(i%2)*190,86+(i/2)*105,180,96};
            g.hero={434,86,800,249};g.shelf={434,350,800,269};
        }
    }else if(style==Hybrid){
        for(int i=0;i<6;++i)g.nav[i]={44+i*200,83,190,77};
        for(int i=6;i<Slots;++i)g.nav[i]={44,179+(i-6)*106,215,96};
        g.hero={275,179,959,251};g.shelf={275,443,959,176};
    }else if(style==BladeNeo){
        for(int i=0;i<6;++i)g.nav[i]={44+i*200,83,190,48};
        for(int i=6;i<Slots;++i)g.nav[i]={44,150+(i-6)*116,300,106};
        g.hero={360,150,874,282};g.shelf={360,445,874,174};
    }else if(style==Immersive||style==Arcade){
        for(int i=0;i<Slots;++i)g.nav[i]={44+i*120,539,112,80};
        g.hero={44,83,1190,style==Immersive?310:222};
        g.shelf={44,style==Immersive?406:318,1190,style==Immersive?120:208};
    }else{
        for(int i=0;i<Slots;++i)g.nav[i]={44+i*120,83,112,83};
        g.hero={44,181,1190,247};g.shelf={44,441,1190,178};
    }
    return g;
}
inline int Move(const Geometry& g,int row,int direction,bool disc=false){
    const int count=Slots+(disc?1:0);
    if(row<0||row>=count||direction<0||direction>3)return 0;
    const auto a=row==DiscSlot?g.hero:g.nav[row];const int ax=2*a.x+a.w,ay=2*a.y+a.h;
    int best=row;long bestScore=0x7fffffff;
    for(int i=0;i<count;++i){
        if(i==row)continue;const auto b=i==DiscSlot?g.hero:g.nav[i];
        const int dx=2*b.x+b.w-ax,dy=2*b.y+b.h-ay;
        const int forward=direction==0?-dx:direction==1?dx:direction==2?-dy:dy;
        if(forward<=0)continue;
        const int lateral=direction<2?std::abs(dy):std::abs(dx);
        const long score=long(forward)+long(lateral)*4;
        if(score<bestScore){best=i;bestScore=score;}
    }
    return best;
}
}
