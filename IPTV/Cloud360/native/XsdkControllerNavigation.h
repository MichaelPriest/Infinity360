#pragma once
#include "ControllerNavigation.h"
#include "xsdk/xam.h"
namespace XsdkControllerNavigation {
static_assert(INPUT_GAMEPAD_DPAD_UP==ControllerNavigation::Up&&INPUT_GAMEPAD_DPAD_DOWN==ControllerNavigation::Down&&INPUT_GAMEPAD_DPAD_LEFT==ControllerNavigation::Left&&INPUT_GAMEPAD_DPAD_RIGHT==ControllerNavigation::Right,"Unexpected direction bits");
// Read all four XAM slots. Active controller wins, then last connected slot.
inline bool Poll(INPUT_STATE& input){
    static int selected=-1;
    INPUT_STATE states[4]{};bool connected[4]{},active[4]{};
    for(unsigned i=0;i<4;++i){
        connected[i]=XamInputGetState(i,0,&states[i])==ERROR_SUCCESS;
        if(connected[i]){
            const auto& g=states[i].gamepad;
            active[i]=(g.buttons&ControllerNavigation::KnownButtons)||g.thumb_lx>=16000||g.thumb_lx<=-16000||g.thumb_ly>=16000||g.thumb_ly<=-16000||g.left_trigger>30||g.right_trigger>30;
        }
    }
    selected=ControllerNavigation::Choose(selected,connected,active);
    input=selected>=0?states[selected]:INPUT_STATE{};
    return selected>=0;
}
inline ControllerNavigation::Frame Read(ControllerNavigation::State& nav,const INPUT_STATE& input,bool connected,bool repeat=true){
    return nav.Poll(connected,input.gamepad.buttons,input.gamepad.thumb_lx,input.gamepad.thumb_ly,ControllerNavigation::Now(),repeat);
}
}
