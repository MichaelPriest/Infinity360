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
        if (state == 0) {
            if (XamUserGetSigninState(i) == 0) result.slots[i] = FromXam(0, "");
            continue;
        }
        if (state != 1 && state != 2) continue;
        USER_SIGNIN_INFO first{}, second{};
        if (XamUserGetSigninInfo(i, 0, &first) != 0 ||
            first.signin_state != state || first.xuid == 0) continue;
        if (XamUserGetSigninInfo(i, 0, &second) != 0 ||
            second.signin_state != state || second.xuid != first.xuid ||
            XamUserGetSigninState(i) != state) continue;
        // Identity is compared in memory only; never retained, logged or displayed.
        std::size_t length = 0;
        while (length < sizeof(second.name) && second.name[length] != '\0') ++length;
        result.slots[i] = FromXam(state, std::string(second.name, length));
    }
    return result;
}
}
#endif
