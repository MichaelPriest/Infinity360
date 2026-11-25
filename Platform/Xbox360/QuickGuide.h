#pragma once
#include <string>
#include <algorithm>

// Dashboard-local guide. Never intercepts the system Guide button in a game.
namespace QuickGuide {
enum Action { Games, Recent, Pinned, TV, Notifications, Transfers, Settings, Store, Count };
inline const char* Default(){return "01234567";}
inline bool Valid(const std::string& s){
    if(s.size()!=Count)return false;
    unsigned seen=0;
    for(char ch:s){
        if(ch<'0'||ch>='0'+Count)return false;
        const unsigned bit=1u<<(ch-'0');if(seen&bit)return false;seen|=bit;
    }
    return seen==(1u<<Count)-1;
}
inline bool Move(std::string& draft,int& row,int direction){
    if(!Valid(draft)||row<0||row>=Count||(direction!=1&&direction!=-1))return false;
    const int next=row+direction;if(next<0||next>=Count)return false;
    std::swap(draft[row],draft[next]);row=next;return true;
}
inline const char* Label(unsigned action){
    static const char* const names[]={"Biblioteca","Recentes","Fixados e favoritos","Cloud360 IPTV","Notificacoes","Downloads","Configuracoes","Loja"};
    return names[action<Count?action:0];
}
}
