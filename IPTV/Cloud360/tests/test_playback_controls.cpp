#include "playback_controls.h"
#include "read_clock.h"
int main(){
    using C=CloudPlaybackControls;
    C local(true,C::Pause);
    if(local.Update(C::Pause,100)||local.Paused())return 1;
    local.Update(0,200);
    if(!local.Update(C::Pause,300)||!local.Paused()||local.Clock(5000)!=300)return 2;
    if(local.Update(C::Pause,6000))return 3;
    local.Update(0,7000);
    if(!local.Update(C::Pause,8000)||local.Paused()||local.Clock(8000)!=300||local.Clock(8100)!=400)return 4;
    for(int i=0;i<12;++i){local.Update(0,9000+i*2);local.Update(C::Quieter,9001+i*2);}
    if(local.Volume()!=0||local.Gain()!=0)return 5;
    local.Update(0,10000);local.Update(C::Louder,10001);
    if(local.Volume()!=10||local.Gain()<0.099f||local.Gain()>0.101f)return 6;
    local.Update(C::Mute,10002);
    if(!local.Muted()||local.Gain()!=0||local.Volume()!=10)return 7;
    local.Update(0,10003);local.Update(C::Mute,10004);
    if(local.Muted()||local.Gain()==0)return 8;
    C remote(false);if(remote.Update(C::Pause,100)||remote.Paused()||remote.Clock(200)!=200)return 9;
    // Paused time is removed before applying synchronous read compensation.
    C combined(true);CloudReadClock clock;
    if(!clock.Observe(1000,101000))return 10;
    combined.Update(C::Pause,200000);combined.Update(0,300000);
    combined.Update(C::Pause,1200000);
    if(clock.Elapsed(combined.Clock(1200000))!=100000)return 11;
    C restarted(false);restarted.RestoreAudio(local.Audio());
    if(restarted.Volume()!=10||restarted.Muted()||restarted.Clock(200)!=200)return 12;
    restarted.RestoreAudio(C::AudioPreference{150,true});if(restarted.Volume()!=100||restarted.Gain()!=0)return 13;
    CloudQualityMenu menu;unsigned current=800000;
    if(menu.Open({400000},current)||menu.Active())return 14;
    if(!menu.Open({800000,400000,800000,0},current)||menu.Count()!=3||menu.Choice()!=800000)return 15;
    menu.Move(1);if(menu.Choice()!=800000)return 16;
    menu.Move(-1);if(menu.Choice()!=400000||current!=800000)return 17;
    menu.Close();if(menu.Confirm(current)||current!=800000)return 18;
    if(!menu.Open({400000,800000},current))return 19;
    menu.Move(-1);if(!menu.Confirm(current)||current!=400000||menu.Active())return 20;
    if(!menu.Open({400000,800000},current)||menu.Confirm(current))return 21;
    if(!menu.Open({400000,800000},current))return 22;
    menu.Move(-1);menu.Move(-1);if(menu.Choice()!=0||!menu.Confirm(current)||current)return 23;
    CloudChannelCursor channels;
    if(channels.Open(1,0)||channels.Open(3,3)||channels.Active())return 24;
    if(!channels.Open(3,1)||channels.Index()!=1)return 25;
    channels.Move(1);channels.Move(1);if(channels.Index()!=2)return 26;
    channels.Move(-1);channels.Move(-1);channels.Move(-1);if(channels.Index()!=0)return 27;
    channels.Close();channels.Move(1);if(channels.Active()||channels.Index())return 28;
    return 0;
}
