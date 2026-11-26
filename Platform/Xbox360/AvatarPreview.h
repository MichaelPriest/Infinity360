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
constexpr std::size_t AvatarPngLimit=65536;
inline std::uint32_t AvatarBE32(const unsigned char* p){
    return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|(std::uint32_t(p[2])<<8)|p[3];
}
// Validate container bounds before passing trusted-system PNG to the native decoder.
// This is a structural check, not an independent PNG decompressor.
inline bool InspectAvatarPng(const unsigned char* p,std::size_t n,unsigned& w,unsigned& h){
    w=h=0;
    constexpr unsigned char sig[]={137,80,78,71,13,10,26,10};
    if(!p||n<45||n>AvatarPngLimit||!std::equal(sig,sig+8,p))return false;
    bool ihdr=false,idat=false;
    unsigned width=0,height=0;
    std::size_t off=8;
    while(n-off>=12){
        const auto length=AvatarBE32(p+off),type=AvatarBE32(p+off+4);
        if(length>n-off-12)return false;
        const auto* data=p+off+8;
        if(!ihdr){
            if(type!=0x49484452u||length!=13)return false;
            width=AvatarBE32(data);height=AvatarBE32(data+4);
            if(!width||!height||width>AvatarFrameMaxWidth||height>AvatarFrameMaxHeight||
               data[8]!=8||(data[9]!=2&&data[9]!=6)||data[10]||data[11]||data[12])return false;
            ihdr=true;
        }else if(type==0x49484452u)return false;
        else if(type==0x49444154u){if(length)idat=true;}
        else if(type==0x49454e44u){
            if(length||!idat||off+12!=n)return false;
            w=width;h=height;return true;
        }
        off+=std::size_t(length)+12;
    }
    return false;
}
inline bool DecodeAvatarArgb(const unsigned char* p,std::size_t n,unsigned w,unsigned h,unsigned pitch,
                             std::vector<unsigned char>& rgba){
    rgba.clear();
    if(!p||!w||!h||w>AvatarFrameMaxWidth||h>AvatarFrameMaxHeight||pitch<w*4u||
       pitch>AvatarFrameMaxWidth*4u+127u||n!=std::size_t(pitch)*h)return false;
    rgba.resize(std::size_t(w)*h*4);
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
        const auto* a=p+std::size_t(y)*pitch+x*4;
        auto* b=rgba.data()+4*(std::size_t(y)*w+x);
        b[0]=a[1];b[1]=a[2];b[2]=a[3];b[3]=a[0];
    }
    if(!ValidAvatarFrame(w,h,rgba)){rgba.clear();return false;}
    return true;
}
class AvatarPreview {
    Header owner_;
    unsigned width_=0,height_=0;
    std::vector<unsigned char> rgba_;
public:
    void Clear(){owner_=Header{};width_=height_=0;rgba_.clear();}
    bool KnownFor(const Header& h)const{return SameArtAccount(owner_,h)&&!rgba_.empty();}
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
