#pragma once
#include "SocialProfile.h"
#include <cstdint>

namespace InfinitySocial {
// XPROFILE_GAMERCARD_CRED (INT32); XAM returns a 32-bit big-endian ABI.
// Only accept a single global setting for the requested user, inside the buffer.
constexpr std::uint32_t GamerscoreSetting = 0x10040006u;
constexpr std::uint32_t AchievementsSetting = 0x10040013u;
inline std::uint32_t ProfileBE32(const unsigned char* p) {
    return (std::uint32_t(p[0]) << 24) | (std::uint32_t(p[1]) << 16) |
           (std::uint32_t(p[2]) << 8) | std::uint32_t(p[3]);
}
inline bool ParseProfileCounter(const unsigned char* data, std::size_t bytes,
                           std::uint32_t base, std::uint32_t user, std::uint32_t id,unsigned& score) {
    if(id!=GamerscoreSetting&&id!=AchievementsSetting)return false;
    if (!data || bytes < 48 || base > 0xfffffff7u || user >= 4) return false;
    if (ProfileBE32(data) != 1 || ProfileBE32(data + 4) != base + 8) return false;
    const auto* setting = data + 8;
    if (ProfileBE32(setting) != 1 || ProfileBE32(setting + 8) != user ||
        ProfileBE32(setting + 16) != id || setting[24] != 1) return false;
    const auto value = ProfileBE32(setting + 32);
    if (value > 0x7fffffffu) return false;
    score = value;
    return true;
}
inline bool ParseGamerscore(const unsigned char* data,std::size_t bytes,std::uint32_t base,std::uint32_t user,unsigned& score){
    return ParseProfileCounter(data,bytes,base,user,GamerscoreSetting,score);
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
#include <xsdk/kernel.h>
#include <fstream>
namespace InfinitySocial {
inline bool GamerpicEnabled(const std::string& dir) {
    std::ifstream flag(dir+"enable-native-gamerpic.txt");
    std::string line;
    return std::getline(flag,line)&&(line=="ENABLE-NATIVE-GAMERPIC"||line=="ENABLE-NATIVE-GAMERPIC\r");
}
// Read-only, fixed-size request; no writes or remote XUID lookup.
inline bool ReadXsdkCounter(std::uint32_t user,std::uint32_t id,unsigned& score) {
    alignas(8) unsigned char buffer[128] = {};
    std::uint32_t bytes = sizeof(buffer);
    if (XamUserReadProfileSettings(0, user, 0, nullptr, 1, &id, &bytes,
                                   buffer, nullptr) != 0 || bytes > sizeof(buffer)) return false;
    return ParseProfileCounter(buffer, bytes,
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(buffer)), user,id,score);
}
// Call from the dashboard thread on a bounded refresh interval.
// The caller must not equate online sign-in with a verified chat service.
inline Snapshot ReadXsdkProfiles(bool readPicture=false,int preferred=-1) {
    Snapshot result;
    std::array<std::uint64_t,4> identities{};
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
        const bool scoreKnown = ReadXsdkCounter(i,GamerscoreSetting,score);
        if (XamUserGetSigninInfo(i, 0, &second) != 0 ||
            second.signin_state != state || second.xuid != first.xuid ||
            XamUserGetSigninState(i) != state) continue;
        // Identity retained only in RAM for safe comparison, never logged or displayed.
        std::size_t length = 0;
        while (length < sizeof(second.name) && second.name[length] != '\0') ++length;
        result.slots[i] = FromXam(state, std::string(second.name, length));
        result.slots[i].gamerscoreKnown = scoreKnown;
        if (scoreKnown) result.slots[i].gamerscore = score;
        identities[i]=second.xuid;
        result.slots[i].identity=second.xuid;
    }
    // One selected user's picture per refresh, only with explicit opt-in.
    // ABI is experimental: do not call when the export is missing.
    const auto selected=Resolve(result,preferred);
    if(selected.controller<0||selected.guest)return result;
    const auto user=static_cast<std::uint32_t>(selected.controller);
    unsigned achievements=0;
    const bool known=ReadXsdkCounter(user,AchievementsSetting,achievements);
    USER_SIGNIN_INFO verified{};
    const auto expectedState=selected.state==SignIn::Online?2u:1u;
    if(XamUserGetSigninInfo(user,0,&verified)!=0||verified.xuid!=identities[user]||
       verified.signin_state!=expectedState||XamUserGetSigninState(user)!=expectedState){
        result.slots[user]=Slot{};return result;
    }
    result.slots[user].achievementsKnown=known;
    if(known)result.slots[user].achievements=achievements;
    if(!readPicture)return result;
    HANDLE module=0;
    void* address=nullptr;
    if(XexGetModuleHandle("xam.xex",&module)!=0||!module||
       XexGetProcedureAddress(module,0x2f5u,&address)!=0||!address)return result;
    USER_SIGNIN_INFO before{},after{};
    const auto state=selected.state==SignIn::Online?2u:1u;
    if(XamUserGetSigninInfo(user,0,&before)!=0||before.xuid!=identities[user]||
       before.signin_state!=state){result.slots[user]=Slot{};return result;}
    // Xenia Canary reference: tile type 2 (64x64 gamer tile), title/item 0,
    // stride in bytes, height in rows, synchronous null overlapped.
    alignas(128) std::array<unsigned char,GamerpicBytes> buffer{};
    using ReadTile=std::uint32_t(*)(std::uint32_t,std::uint32_t,std::uint64_t,
        std::uint32_t,void*,std::uint32_t,std::uint32_t,XAM_OVERLAPPED*);
    const auto rc=reinterpret_cast<ReadTile>(address)(2u,0u,0u,user,buffer.data(),
        static_cast<std::uint32_t>(GamerpicSide*4),static_cast<std::uint32_t>(GamerpicSide),nullptr);
    if(XamUserGetSigninInfo(user,0,&after)!=0||after.xuid!=before.xuid||
       after.signin_state!=state||XamUserGetSigninState(user)!=state){
        result.slots[user]=Slot{};return result;
    }
    if(rc==0)DecodeGamerpic(buffer.data(),buffer.size(),result.slots[user].gamerpic);
    return result;
}
}
#endif
