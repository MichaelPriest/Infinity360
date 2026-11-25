#pragma once
#include <cstdio>
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_POWER)
extern "C" void HalReturnToFirmware(unsigned int routine);
#endif

// Only normal power-down and quiescing reboot are exposed; no forced resets.
struct ConsolePower {
    enum Action { Shutdown, Reboot };
    static bool Supported(){
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_POWER)
        return true;
#else
        return false;
#endif
    }
    // Kernel FIRMWARE_REENTRY: HalPowerDownRoutine / HalRebootQuiesceRoutine.
    static unsigned Routine(Action a){return a==Shutdown?5u:a==Reboot?6u:0u;}
    enum Result { Unavailable, Busy, Armed, Requested, Returned };
    struct Confirmation {
        Action action=Shutdown;
        bool armed=false;
        void Select(Action next){action=next;armed=false;}
        void Cancel(){armed=false;}
        Result Accept(bool supported,bool busy){
            if(!supported){Cancel();return Unavailable;}
            if(busy){Cancel();return Busy;}
            if(!Routine(action)){Cancel();return Unavailable;}
            if(!armed){armed=true;return Armed;}
            Cancel();return Requested;
        }
    };
    // Unexpected return is NOT success. Never call this on host or default XSDK.
    static Result Invoke(Action action){
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_POWER)
        const unsigned routine=Routine(action);
        if(!routine)return Unavailable;
        if(std::fflush(nullptr)!=0)return Busy;
        HalReturnToFirmware(routine);
        return Returned;
#else
        (void)action;return Unavailable;
#endif
    }
};
