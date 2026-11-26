#pragma once
#include "NativeSocialSignals.h"
#include <xsdk/kernel.h>
#include <fstream>

namespace InfinitySocial {
// Optional read-only subscription to title-visible XAM notifications.
// Separate from xsdk_system_ui_open's system listener.
class XsdkSocialListener {
    HANDLE listener_=0;
    bool enabled_=false,polled_=false;
    std::uint64_t identity_=0,lastPoll_=0;
    int controller_=-1;
    SignalGate gate_;
    void Close(){
        if(listener_)NtClose(listener_);
        listener_=0;identity_=0;controller_=-1;gate_=SignalGate{};
    }
    static bool Verify(const Header& h){
        if(!EligibleForSocialSignals(h))return false;
        USER_SIGNIN_INFO info{};
        const auto slot=static_cast<std::uint32_t>(h.controller);
        return XamUserGetSigninInfo(slot,0,&info)==0&&info.xuid==h.identity&&
               info.signin_state==2&&XamUserGetSigninState(slot)==2;
    }
public:
    explicit XsdkSocialListener(const std::string& dir){
        std::ifstream f(dir+"enable-native-social-events.txt");
        std::string line;
        enabled_=std::getline(f,line)&&(line=="ENABLE-NATIVE-SOCIAL-EVENTS"||line=="ENABLE-NATIVE-SOCIAL-EVENTS\r");
    }
    ~XsdkSocialListener(){Close();}
    XsdkSocialListener(const XsdkSocialListener&)=delete;
    XsdkSocialListener& operator=(const XsdkSocialListener&)=delete;
    template<class Report> bool Poll(const Header& h,std::uint64_t now,bool allowed,Report report){
        if(!enabled_)return false;
        if(!allowed||!EligibleForSocialSignals(h)){Close();polled_=false;return false;}
        if(listener_&&(identity_!=h.identity||controller_!=h.controller)){Close();polled_=false;}
        if(polled_&&now>=lastPoll_&&now-lastPoll_<1000)return false;
        polled_=true;lastPoll_=now;
        if(!Verify(h)){Close();return false;}
        if(!listener_){
            listener_=XamNotifyCreateListener(SocialListenerMask,0);
            if(!listener_)return false;
            identity_=h.identity;controller_=h.controller;
            // Do not report anything on the first poll.
            return false;
        }
        std::array<std::uint32_t,8> ids{};
        unsigned count=0;
        for(;count<ids.size();++count){
            std::uint32_t id=0,ignored=0;
            if(!XNotifyGetNext(listener_,0,&id,&ignored))break;
            ids[count]=id;
        }
        // Drop the batch if the active account changed during the dequeue.
        if(!Verify(h)){Close();return false;}
        bool changed=false;
        for(unsigned i=0;i<count;++i){
            if(const char* label=gate_.Observe(ids[i],now)){report(label);changed=true;}
        }
        return changed;
    }
};
}
