#pragma once
#include "SocialProfile.h"
#include <array>
namespace InfinitySocial {
// Legacy Xbox privilege IDs, not modern console party/voice entitlements.
enum class Privilege { Communications, FriendsCommunications, ProfileViewing, FriendsProfileViewing, Multiplayer, Count };
constexpr std::array<std::uint32_t,5> PrivilegeIds={{252u,251u,249u,248u,254u}};
enum class PrivilegeValue { Unknown, Denied, Granted };
enum class AccessScope { Unknown, Denied, FriendsOnly, Granted };
inline PrivilegeValue DecodePrivilege(std::uint32_t status,std::uint32_t value){
    if(status||value>1)return PrivilegeValue::Unknown;
    return value?PrivilegeValue::Granted:PrivilegeValue::Denied;
}
inline AccessScope Scope(PrivilegeValue all,PrivilegeValue friends){
    if(all==PrivilegeValue::Granted)return AccessScope::Granted;
    if(all==PrivilegeValue::Unknown)return AccessScope::Unknown;
    if(friends==PrivilegeValue::Granted)return AccessScope::FriendsOnly;
    return friends==PrivilegeValue::Denied?AccessScope::Denied:AccessScope::Unknown;
}
inline const char* AccessLabel(AccessScope scope){
    switch(scope){case AccessScope::Granted:return "Permitido";
        case AccessScope::FriendsOnly:return "Somente amigos";
        case AccessScope::Denied:return "Restrito";default:return "Nao confirmado";}
}
using PrivilegeValues=std::array<PrivilegeValue,PrivilegeIds.size()>;
class ProfilePrivileges {
    Header owner_;
    PrivilegeValues values_{};
    std::uint64_t at_=0;
    bool known_=false;
public:
    static constexpr std::uint64_t ValidMs=60000;
    void Clear(){owner_=Header{};values_.fill(PrivilegeValue::Unknown);at_=0;known_=false;}
    bool Matches(const Header& h)const{return known_&&SameArtAccount(owner_,h)&&h.state==SignIn::Online;}
    bool Fresh(const Header& h,std::uint64_t now)const{return Matches(h)&&now>=at_&&now-at_<ValidMs;}
    bool Expire(const Header& h,std::uint64_t now){if(known_&&!Fresh(h,now)){Clear();return true;}return false;}
    void Observe(const Header& h){if(!Matches(h))Clear();}
    bool Set(const Header& before,const Header& after,const PrivilegeValues& values,std::uint64_t now){
        Clear();
        if(!SameArtAccount(before,after)||after.state!=SignIn::Online)return false;
        owner_.origin=after.origin;owner_.state=after.state;owner_.controller=after.controller;
        owner_.identity=after.identity;owner_.guest=after.guest;
        values_=values;at_=now;known_=true;return true;
    }
    AccessScope Communications()const{return Scope(values_[0],values_[1]);}
    AccessScope Profiles()const{return Scope(values_[2],values_[3]);}
    AccessScope Multiplayer()const{return Scope(values_[4],PrivilegeValue::Denied);}
};
}
