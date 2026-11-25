#pragma once
#include <algorithm>
#include <string>
namespace GameGrid {
enum Preference { Auto, List, Covers, Count };
constexpr int Columns=6, Rows=2, Page=Columns*Rows;
inline bool Parse(const std::string& s,unsigned& out){
    if(s.size()!=1||s[0]<'0'||s[0]>'2')return false;
    out=unsigned(s[0]-'0');return true;
}
inline bool Enabled(unsigned preference,unsigned layout){
    return preference==Covers||(preference==Auto&&layout!=0);
}
inline const char* Name(unsigned p){return p==List?"Lista":p==Covers?"Grade de capas":"Automatico";}
inline int Move(int row,int count,int dir){
    if(count<=0)return 0;
    row=std::max(0,std::min(row,count-1));
    if(dir==0&&row%Columns>0)return row-1;
    if(dir==1&&row%Columns<Columns-1&&row+1<count)return row+1;
    if(dir==2&&row>=Columns)return row-Columns;
    if(dir==3&&((row/Columns+1)*Columns)<count)return std::min(row+Columns,count-1);
    return row;
}
}
