#pragma once
#include "HomeCanvas.h"
#include <algorithm>
#include <vector>
namespace InfinityButtons {
enum Kind { A,B,X,Y,LB,RB,LT,RT,Start,Back,L3,R3,Dpad,Guide,Count };
inline int Width(int kind,int h){return kind>=LB&&kind<=Back?h*3/2:h;}
inline void Draw(HomeCanvas& c,int kind,int x,int y,int h){c.ControllerButton(kind,x,y,h);}
inline void Hint(HomeCanvas& c,int kind,int x,int y,int h,const std::string& text,int maxWidth,unsigned color){
    Draw(c,kind,x,y,h);c.TextFit(x+Width(kind,h)+5,y,text,h, color,maxWidth);
}
// Shared command strip: parse only explicitly delimited key hints, never arbitrary prose.
struct Command {int first=-1,second=-1;std::string label;};
inline std::string Trim(const std::string& s){
    const auto a=s.find_first_not_of(" \t"),b=s.find_last_not_of(" \t");
    return a==std::string::npos?std::string():s.substr(a,b-a+1);
}
inline Command ParseCommand(const std::string& raw){
    Command out;out.label=Trim(raw);
    const char* keys[]={"A","B","X","Y","LB","RB","LT","RT","START","BACK","L3","R3","Direcional","Guide","Start","RS","LB/RB","Esq/Dir","Cima/Baixo","Direcoes","A/X"};
    const int kinds[]={A,B,X,Y,LB,RB,LT,RT,Start,Back,L3,R3,Dpad,Guide,Start,R3,LB,Dpad,Dpad,Dpad,A};
    for(int i=0;i<21;++i){
        const std::string key=keys[i];
        if(out.label.size()>key.size()&&out.label.compare(0,key.size(),key)==0&&(out.label[key.size()]==' '||out.label[key.size()]==':')){
            out.first=kinds[i];out.second=i==16?RB:i==20?X:-1;out.label=Trim(out.label.substr(key.size()+1));break;
        }
    }
    return out;
}
inline std::vector<Command> ParseCommands(const std::string& text){
    std::vector<Command> items;size_t begin=0;
    while(begin<=text.size()){
        const auto end=text.find('|',begin);
        auto item=ParseCommand(text.substr(begin,end==std::string::npos?end:end-begin));
        if(!item.label.empty())items.push_back(item);
        if(end==std::string::npos)break;begin=end+1;
    }
    return items;
}
inline void Strip(HomeCanvas& c,int x,int y,const std::string& text,int size,unsigned color,int maxWidth){
    const auto items=ParseCommands(text);
    if(items.empty()||maxWidth<=0)return;
    const int h=std::max(16,std::min(27,size+4)),gap=8;
    std::vector<int> widths;int total=0;
    for(const auto& item:items){
        const int glyph=(item.first<0?0:Width(item.first,h)+5)+(item.second<0?0:Width(item.second,h)+3);
        const int wanted=glyph+std::max(size*2,int(item.label.size())*size/2)+gap;
        widths.push_back(wanted);total+=wanted;
    }
    int used=0;
    for(size_t i=0;i<items.size();++i){
        const auto& item=items[i];
        const int allotted=total<=maxWidth?widths[i]:(i+1==items.size()?maxWidth-used:widths[i]*maxWidth/total);
        int pos=x+used,end=pos+allotted-gap;
        if(item.first>=0&&pos+Width(item.first,h)<=end){Draw(c,item.first,pos,y,h);pos+=Width(item.first,h)+3;}
        if(item.second>=0&&pos+Width(item.second,h)<=end){Draw(c,item.second,pos,y,h);pos+=Width(item.second,h)+3;}
        if(end>pos)c.TextFit(pos,y+2,item.label,size,color,end-pos);
        used+=allotted;
    }
}
}
