#pragma once
#include <cstdint>
#include <algorithm>

// Time-based and bounded; no additional framebuffer or background activity.
// A final redraw at 100% ensures the last frame never keeps a partial effect.
class DashboardMotion {
    struct Track {
        uint64_t start=0; bool running=false;
        void Begin(uint64_t now){start=now;running=true;}
        unsigned Sample(uint64_t now,unsigned duration,bool& redraw){
            if(!running)return 100;
            redraw=true;
            if(now<start||now-start>=duration){running=false;return 100;}
            const unsigned t=unsigned((now-start)*100/duration);
            return 100-(100-t)*(100-t)/100;
        }
        void Clear(){running=false;}
    };
    Track focus_,scene_;
    uint64_t sceneKey_=0;bool known_=false;
public:
    struct Frame{unsigned focus=100,scene=100;bool redraw=false;};
    void Observe(uint64_t now,uint64_t key,bool input,bool enabled){
        const bool changed=known_&&key!=sceneKey_;
        known_=true;sceneKey_=key;
        if(!enabled){focus_.Clear();scene_.Clear();return;}
        if(input)focus_.Begin(now);
        if(changed)scene_.Begin(now);
    }
    Frame Sample(uint64_t now,bool enabled){
        Frame f;
        if(!enabled){f.redraw=focus_.running||scene_.running;focus_.Clear();scene_.Clear();return f;}
        f.focus=focus_.Sample(now,150,f.redraw);f.scene=scene_.Sample(now,220,f.redraw);
        return f;
    }
};
