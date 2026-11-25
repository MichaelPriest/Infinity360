#pragma once
#include "HomeCanvas.h"
#include <cstdint>
#include <chrono>
#include <string>

class IdleScreen {
public:
    static bool Valid(unsigned minutes){return minutes==0||minutes==1||minutes==3||minutes==5||minutes==10;}
    static bool Parse(const std::string& s,unsigned& minutes){
        if(s=="0"||s=="1"||s=="3"||s=="5"||s=="10"){
            minutes=s=="10"?10:static_cast<unsigned>(s[0]-'0');return true;
        }return false;
    }
    static unsigned Next(unsigned value){
        switch(value){case 0:return 1;case 1:return 3;case 3:return 5;case 5:return 10;default:return 0;}
    }
    static uint64_t Now(){return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
    // A change in state or a new animation step requires a texture upload.
    bool Update(uint64_t now,unsigned minutes,bool eligible,bool input){
        const bool before=active_;
        const auto oldStep=step_;
        if(!initialized_||now<last_||minutes!=minutes_||!eligible||input||!Valid(minutes)||!minutes){
            initialized_=true;last_=now;minutes_=minutes;active_=false;step_=0;
        }else if(now-last_>=uint64_t(minutes)*60000){
            active_=true;step_=(now-last_-uint64_t(minutes)*60000)/1000;
        }
        return before!=active_||(active_&&oldStep!=step_);
    }
    bool Active()const{return active_;}
    uint64_t Step()const{return step_;}
    static void Draw(HomeCanvas& c,uint64_t step){
        c.SetTheme(0);c.Box(0,0,1280,720,0xff000000u);
        auto bounce=[](uint64_t v,unsigned span){const auto m=v%(span*2u);return static_cast<int>(m<=span?m:span*2u-m);};
        const int x=70+bounce((step%1600)*13,760),y=80+bounce((step%800)*7,400);
        c.Box(x,y+9,38,30,0xff406a27u,13);c.Box(x+4,y+13,30,22,0xff000000u,9);
        c.Box(x+29,y+9,38,30,0xff406a27u,13);c.Box(x+33,y+13,30,22,0xff000000u,9);
        c.Text(x+82,y,"INFINITY 360",32,0xff739456u,380);
        c.Text(x+82,y+47,c.Tr("Pressione um botao"),17,0xff829080u,380);
    }
private:
    bool initialized_=false,active_=false;unsigned minutes_=0;uint64_t last_=0,step_=0;
};
