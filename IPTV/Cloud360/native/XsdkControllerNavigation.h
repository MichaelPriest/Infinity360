#pragma once
#include "ControllerNavigation.h"
#include "xsdk/xam.h"
namespace XsdkControllerNavigation {
static_assert(INPUT_GAMEPAD_DPAD_UP==ControllerNavigation::Up&&INPUT_GAMEPAD_DPAD_DOWN==ControllerNavigation::Down&&INPUT_GAMEPAD_DPAD_LEFT==ControllerNavigation::Left&&INPUT_GAMEPAD_DPAD_RIGHT==ControllerNavigation::Right,"Unexpected direction bits");
inline ControllerNavigation::Frame Read(ControllerNavigation::State& nav,const INPUT_STATE& input,bool connected,bool repeat=true){
    return nav.Poll(connected,input.gamepad.buttons,input.gamepad.thumb_lx,input.gamepad.thumb_ly,ControllerNavigation::Now(),repeat);
}
}
