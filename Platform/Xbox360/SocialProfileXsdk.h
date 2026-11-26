#pragma once
#include "SocialProfile.h"
#include <cstdint>

namespace InfinitySocial {
// Portable mapping for XAM state values. Unknown values stay unavailable.
// No service capability, avatar, Gamerscore or guest status is inferred.
inline Slot FromXam(std::uint32_t state, const std::string& name) {
    Slot slot;
    switch (state) {
        case 0: slot.state = SignIn::SignedOut; break;
        case 1: slot.state = SignIn::Local; break;
        case 2: slot.state = SignIn::Online; break;
        default: return slot;
    }
    slot.origin = Origin::Console;
    if (IsSignedIn(slot) && ValidGamertag(name)) slot.gamertag = name;
    return slot;
}
}

#if defined(INFINITY_SOCIAL_XSDK)
#include <xsdk/xam.h>
namespace InfinitySocial {
// Call from the dashboard thread on a bounded refresh interval.
// The caller must not equate online sign-in with a verified chat service.
inline Snapshot ReadXsdkProfiles() {
    Snapshot result;
    for (std::uint32_t i = 0; i < XSDK_XAM_USERS && i < result.slots.size(); ++i) {
        auto state = XamUserGetSigninState(i);
        char name[XSDK_XAM_GAMERTAG_BYTES] = {};
        if ((state == 1 || state == 2) &&
            XamUserGetName(i, name, sizeof(name)) != 0) name[0] = '\0';
        name[sizeof(name)-1] = '\0';
        // Do not associate a name with a slot whose state changed during sampling.
        if (state != XamUserGetSigninState(i)) continue;
        result.slots[i] = FromXam(state, name);
    }
    return result;
}
}
#endif
