#pragma once
#include "SocialProfile.h"
#include <cstdint>

namespace InfinitySocial {
// XPROFILE_GAMERCARD_CRED (INT32); XAM returns a 32-bit big-endian ABI.
// Only accept a single global setting for the requested user, inside the buffer.
constexpr std::uint32_t GamerscoreSetting = 0x10040006u;
inline std::uint32_t ProfileBE32(const unsigned char* p) {
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
           (std::uint32_t(p[2]) << 8) | std::uint32_t(p[3]);
}
inline bool ParseGamerscore(const unsigned char* data, std::size_t bytes,
                           std::uint32_t base, std::uint32_t user, unsigned& score) {
    if (!data || bytes < 48 || base > 0xfffffff7u || user >= 4) return false;
    if (ProfileBE32(data) != 1 || ProfileBE32(data + 4) != base + 8) return false;
    const auto* setting = data + 8;
    if (ProfileBE32(setting) != 1 || ProfileBE32(setting + 8) != user ||
        ProfileBE32(setting + 16) != GamerscoreSetting || setting[24] != 1) return false;
    const auto value = ProfileBE32(setting + 32);
    if (value > 0x7fffffffu) return false;
    score = value;
    return true;
}
// Portable mapping for XAM state values. Unknown values stay unavailable.
// No service capability, avatar or guest status is inferred.
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
// Read-only, fixed-size request; no writes or remote XUID lookup.
inline bool ReadXsdkGamerscore(std::uint32_t user, unsigned& score) {
    const std::uint32_t id = GamerscoreSetting;
    alignas(8) unsigned char buffer[128] = {};
    std::uint32_t bytes = sizeof(buffer);
    if (XamUserReadProfileSettings(0, user, 0, nullptr, 1, &id, &bytes,
                                   buffer, nullptr) != 0 || bytes > sizeof(buffer)) return false;
    return ParseGamerscore(buffer, bytes,
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(buffer)), user, score);
}
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
        unsigned score = 0;
        const bool scoreKnown = ReadXsdkGamerscore(i, score);
        if (XamUserGetSigninInfo(i, 0, &second) != 0 ||
            second.signin_state != state || second.xuid != first.xuid ||
            XamUserGetSigninState(i) != state) continue;
        // Identity is compared in memory only; never retained, logged or displayed.
        std::size_t length = 0;
        while (length < sizeof(second.name) && second.name[length] != '\0') ++length;
        result.slots[i] = FromXam(state, std::string(second.name, length));
        result.slots[i].gamerscoreKnown = scoreKnown;
        if (scoreKnown) result.slots[i].gamerscore = score;
    }
    return result;
}
}
#endif
