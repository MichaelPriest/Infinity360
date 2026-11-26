#pragma once
#include "HomeCanvas.h"
namespace InfinityButtons {
enum Kind { A,B,X,Y,LB,RB,LT,RT,Start,Back,L3,R3,Dpad,Guide,Count };
inline int Width(int kind,int h){return kind>=LB&&kind<=Back?h*3/2:h;}
inline void Draw(HomeCanvas& c,int kind,int x,int y,int h){c.ControllerButton(kind,x,y,h);}
inline void Hint(HomeCanvas& c,int kind,int x,int y,int h,const std::string& text,int maxWidth,unsigned color){
    Draw(c,kind,x,y,h);c.TextFit(x+Width(kind,h)+5,y,text,h, color,maxWidth);
}
}
