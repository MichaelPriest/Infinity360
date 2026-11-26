#pragma once
#include "HomeCanvas.h"
namespace InfinityButtons {
enum Kind { A,B,X,Y,LB,RB,LT,RT,Start,Back,L3,R3,Dpad,Guide,Count };
inline int Width(int kind,int h){return kind>=LB&&kind<=Back?h*3/2:h;}
inline void Draw(HomeCanvas& c,int kind,int x,int y,int h){
    c.ControllerButton(kind,x,y,h);
    // Keep the original Microsoft image; add a crisp high-contrast label in small UI sizes.
    if(h<16||h>40||kind<0||kind>=Count)return;
    const char* label[]={"A","B","X","Y","LB","RB","LT","RT","START","BACK","L3","R3","",""};
    if(!*label[kind])return;
    const int w=Width(kind,h),size=kind==Start||kind==Back?std::max(10,h/2):kind>=LB?std::max(11,h*3/5):h*3/4;
    const int top=y+(h-size)/2-1;
    const bool light=kind<4||kind==L3||kind==R3;
    const unsigned ink=light?0xffffffffu:0xff101010u,shadow=light?0xff101010u:0xffeeeeeeu;
    // Small offset outline makes lettering readable without changing button hue.
    c.CenterText(x-1,top,w,label[kind],size,shadow);
    c.CenterText(x+1,top,w,label[kind],size,shadow);
    c.CenterText(x,top-1,w,label[kind],size,shadow);
    c.CenterText(x,top+1,w,label[kind],size,shadow);
    c.CenterText(x,top,w,label[kind],size,ink);
}
inline void Hint(HomeCanvas& c,int kind,int x,int y,int h,const std::string& text,int maxWidth,unsigned color){
    Draw(c,kind,x,y,h);c.TextFit(x+Width(kind,h)+5,y,text,h, color,maxWidth);
}
}
