#pragma once
#include "HomeCanvas.h"
#include "ControllerButtons.h"
#include <algorithm>
#include <cstdint>
// Playback-local controls; bounded bottom 120 rows (600 KiB).
class PlaybackHud {
    static constexpr unsigned Rows=120;
    std::vector<unsigned> bottom_;
    bool pinned_=false;
    int64_t until_=0;
public:
    void Frame(const HomeCanvas& c){c.CaptureBottom(Rows,bottom_);}
    void Clear(){bottom_.clear();}
    void Reveal(int64_t now){until_=now+4000000;}
    void Toggle(int64_t now){pinned_=!pinned_;until_=pinned_?now+4000000:now;}
    bool Visible(int64_t now,bool paused)const{return pinned_||paused||now<until_;}
    bool Draw(HomeCanvas& c,int64_t now,bool paused,const std::string& title,const std::string& state,const std::string& commands,int percent=-1)const{
        if(!c.RestoreBottom(Rows,bottom_))return false;
        if(!Visible(now,paused))return true;
        const unsigned white=0xfff2f7f3u,muted=0xffc3d2c9u,accent=0xffa8ff00u;
        c.Translucent(30,608,1220,102,0xff0d2118u,210,10);
        c.TextFit(48,616,title,24,white,480);c.TextFit(542,619,state,18,muted,686);
        if(percent>=0){c.Box(48,650,1180,6,0xff4c5e54u,3);const int fill=1180*std::min(100,percent)/100;if(fill)c.Box(48,650,fill,6,accent,3);}
        InfinityButtons::Strip(c,48,675,commands+" | L3 Controles",18,white,1180);
        return true;
    }
};
