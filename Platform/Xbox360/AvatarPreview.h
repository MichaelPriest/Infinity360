#pragma once
#include "SocialProfile.h"
#include <array>
#include <algorithm>
namespace InfinitySocial {
// Export presence is not proof of working avatar services or renderer ABI.
constexpr std::array<std::uint32_t,6> AvatarExports={{0x5dcu,0x5ddu,0x5deu,0x5e0u,0x5e1u,0x5e7u}};
struct AvatarCapabilities {
    bool checked=false;
    std::array<bool,AvatarExports.size()> found{};
    std::size_t Count()const{return std::count(found.begin(),found.end(),true);}
    bool AllExports()const{return checked&&Count()==AvatarExports.size();}
};
// A provider must supply already rendered RGBA frames, not raw avatar metadata.
constexpr unsigned AvatarFrameMaxWidth=256,AvatarFrameMaxHeight=384;
inline bool ValidAvatarFrame(unsigned w,unsigned h,const std::vector<unsigned char>& rgba){
    if(!w||!h||w>AvatarFrameMaxWidth||h>AvatarFrameMaxHeight||rgba.size()!=std::size_t(w)*h*4)return false;
    for(std::size_t i=3;i<rgba.size();i+=4)if(rgba[i])return true;
    return false;
}
class AvatarPreview {
    Header owner_;
    unsigned width_=0,height_=0;
    std::vector<unsigned char> rgba_;
public:
    void Clear(){owner_=Header{};width_=height_=0;rgba_.clear();}
    bool KnownFor(const Header& h)const{return SameArtAccount(owner_,h)&&ValidAvatarFrame(width_,height_,rgba_);}
    void Observe(const Header& h){if(!KnownFor(h))Clear();}
    bool Set(const Header& before,const Header& after,unsigned w,unsigned h,const std::vector<unsigned char>& rgba){
        Clear();
        if(!SameArtAccount(before,after)||!ValidAvatarFrame(w,h,rgba))return false;
        // Keep identity only, never duplicate other profile fields or artwork.
        owner_.origin=after.origin;owner_.state=after.state;owner_.controller=after.controller;
        owner_.identity=after.identity;owner_.guest=after.guest;
        width_=w;height_=h;rgba_=rgba;return true;
    }
    unsigned Width()const{return width_;}
    unsigned Height()const{return height_;}
    const std::vector<unsigned char>& Pixels()const{return rgba_;}
};
}
