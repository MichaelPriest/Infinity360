#pragma once
#include "AvatarPreview.h"
namespace InfinitySocial {
// Local-user avatar manifest, based on observed 1000-byte XAM ABI.
// No owner XUID, appearance dimensions, colors or metadata persisted.
constexpr std::size_t AvatarManifestBytes=1000,AvatarComponentOffset=0x120,AvatarComponentBytes=32,AvatarComponentCount=15;
struct AvatarManifestSummary{
    bool known=false;
    unsigned populated=0;
};
inline AvatarManifestSummary InspectAvatarManifest(const unsigned char* p,std::size_t n){
    AvatarManifestSummary out;
    if(!p||n!=AvatarManifestBytes)return out;
    unsigned count=0;
    for(unsigned i=0;i<AvatarComponentCount;++i){
        const auto* id=p+AvatarComponentOffset+i*AvatarComponentBytes;
        if(std::any_of(id,id+16,[](unsigned char v){return v!=0;}))++count;
        else if(i<2)return out; // require both base body and head identifiers
    }
    out.known=true;out.populated=count;return out;
}
// Attempt once per identity, then at most once every 120s. Clock regression
// invalidates cached data; pending manual requests are handled independently.
class AvatarRefresh {
    Header owner_;
    std::uint64_t at_=0;
    bool attempted_=false;
public:
    static constexpr std::uint64_t IntervalMs=120000;
    void Clear(){owner_=Header{};at_=0;attempted_=false;}
    bool Due(const Header& h,std::uint64_t now)const{
        return CanRequestNative(NativeAction::GamerCard,h)&&
            (!attempted_||!SameArtAccount(owner_,h)||now<at_||now-at_>=IntervalMs);
    }
    void Mark(const Header& h,std::uint64_t now){
        Clear();
        if(!CanRequestNative(NativeAction::GamerCard,h))return;
        owner_.origin=h.origin;owner_.state=h.state;owner_.controller=h.controller;owner_.identity=h.identity;owner_.guest=h.guest;
        at_=now;attempted_=true;
    }
    void Observe(const Header& h){if(!SameArtAccount(owner_,h))Clear();}
};
}
