#pragma once
#include <cstdint>
#include <chrono>
namespace ControllerNavigation {
constexpr unsigned Up=1,Down=2,Left=4,Right=8,Mask=15,KnownButtons=0xf3ff;
// Choose a single active controller; never merge actions across players.
inline int Choose(int current,const bool connected[4],const bool active[4]){
    if(current>=0&&current<4&&connected[current]&&active[current])return current;
    for(int i=0;i<4;++i)if(connected[i]&&active[i])return i;
    if(current>=0&&current<4&&connected[current])return current;
    for(int i=0;i<4;++i)if(connected[i])return i;
    return -1;
}
inline uint64_t Now(){return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
struct Frame{unsigned held,edges;};
class State{
    unsigned stick_=0,dir_=0,actions_=0;
    uint64_t next_=0,last_=0;
    bool suppressed_=false;
    unsigned blocked_=0;
    static int absval(int x){return x<0?-x:x;}
    unsigned Analog(int x,int y){
        const int ax=absval(x),ay=absval(y);
        const unsigned h=x<0?Left:Right,v=y<0?Down:Up;
        if(stick_==h&&ax>=11000&&ay<ax+4000)return h;
        if(stick_==v&&ay>=11000&&ax<ay+4000)return v;
        if(ax<16000&&ay<16000)return 0;
        return ax>=ay?h:v;
    }
public:
    void Suppress(){suppressed_=true;blocked_=0;stick_=dir_=actions_=0;next_=last_=0;}
    Frame Poll(bool connected,unsigned raw,int x,int y,uint64_t now,bool repeat=true){
        if(!connected){Suppress();return {0,0};}
        raw&=KnownButtons;
        stick_=Analog(x,y);
        const unsigned d=raw&Mask,h=(d&(Left|Right))==(Left|Right)?0:d&(Left|Right),v=(d&(Up|Down))==(Up|Down)?0:d&(Up|Down);
        unsigned direction=d?(h&&v?(dir_==h?h:v):h|v):stick_;
        unsigned actions=raw&~Mask;
        const unsigned physical=actions|direction;
        if(suppressed_){blocked_=physical;suppressed_=false;return {0,0};}
        blocked_&=physical;actions&=~blocked_;direction&=~blocked_;
        const unsigned held=actions|direction;
        if(now<last_){dir_=direction;actions_=actions;next_=now+380;last_=now;return {held,0};}
        last_=now;
        unsigned edges=actions&~actions_;actions_=actions;
        if(direction!=dir_){dir_=direction;next_=now+380;if(!edges)edges=direction;}
        else if(direction&&repeat&&!edges&&now>=next_){edges=direction;next_=now+115;}
        else if(!repeat||edges)next_=now+380;
        return {held,edges};
    }
};
}
