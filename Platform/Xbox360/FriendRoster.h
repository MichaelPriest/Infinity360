#pragma once
#include "SocialProfile.h"
#include <array>
#include <cstdint>
#include <vector>

namespace InfinitySocial {
// Portable reader for the observed Xbox 360 X_ONLINE_FRIEND record.
// No network call, no persistence, no rich-presence payload, session ID,
// invitation ID or remote XUID returned. Empty is distinct from unavailable.
constexpr std::size_t FriendRecordBytes=0xc4, MaxFriends=100;
enum class FriendState { Unknown, Offline, Online, Playing, RequestSent, RequestReceived };
struct FriendRow {
    std::string name;
    FriendState state=FriendState::Unknown;
    std::uint32_t title=0;
};
inline std::uint32_t FriendBE32(const unsigned char* p){
    return (std::uint32_t(p[0])<<24)|(std::uint32_t(p[1])<<16)|
           (std::uint32_t(p[2])<<8)|std::uint32_t(p[3]);
}
inline FriendState DecodeFriendState(std::uint32_t flags){
    const bool sent=(flags&0x40000000u)!=0,received=(flags&0x80000000u)!=0;
    if(sent&&received)return FriendState::Unknown;
    if(sent)return FriendState::RequestSent;
    if(received)return FriendState::RequestReceived;
    if(flags&1u)return (flags&2u)?FriendState::Playing:FriendState::Online;
    if(flags&2u)return FriendState::Unknown;
    return flags==0?FriendState::Offline:FriendState::Unknown;
}
inline bool ParseFriendRoster(const unsigned char* data,std::size_t bytes,std::size_t count,
                             std::vector<FriendRow>& rows){
    rows.clear();
    if(count>MaxFriends||bytes<count*FriendRecordBytes||bytes>MaxFriends*FriendRecordBytes||(!data&&count))return false;
    std::array<std::uint64_t,MaxFriends> seen{};
    std::vector<FriendRow> parsed;
    parsed.reserve(count);
    for(std::size_t i=0;i<count;++i){
        const auto* p=data+i*FriendRecordBytes;
        const auto xuid=(std::uint64_t(FriendBE32(p))<<32)|FriendBE32(p+4);
        if(!xuid)return false;
        for(std::size_t j=0;j<i;++j)if(seen[j]==xuid)return false;
        seen[i]=xuid;
        std::size_t n=0;while(n<16&&p[8+n])++n;
        if(n==16)return false;
        std::string name(reinterpret_cast<const char*>(p+8),n);
        if(!ValidGamertag(name))return false;
        const auto state=DecodeFriendState(FriendBE32(p+0x18));
        parsed.push_back({name,state,state==FriendState::Playing?FriendBE32(p+0x24):0});
    }
    rows.swap(parsed);
    return true;
}
enum class FriendFilter { All, Online, Requests, Count };
inline bool FriendMatches(FriendState state,FriendFilter filter){
    switch(filter){
        case FriendFilter::All:return true;
        case FriendFilter::Online:return state==FriendState::Online||state==FriendState::Playing;
        case FriendFilter::Requests:return state==FriendState::RequestSent||state==FriendState::RequestReceived;
        default:return false;
    }
}
inline const char* FriendStateLabel(FriendState state){
    switch(state){
        case FriendState::Offline:return "Offline";case FriendState::Online:return "Online";
        case FriendState::Playing:return "Jogando";case FriendState::RequestSent:return "Solicitacao enviada";
        case FriendState::RequestReceived:return "Solicitacao recebida";default:return "Desconhecido";
    }
}
inline std::vector<std::size_t> FilterFriends(const std::vector<FriendRow>& rows,FriendFilter filter){
    std::vector<std::size_t> indices;
    for(std::size_t i=0;i<rows.size();++i)if(FriendMatches(rows[i].state,filter))indices.push_back(i);
    return indices;
}
// Bind a roster provider result to the exact observed profile. The
// snapshot contains display-only rows; identity never enters logs or UI.
class FriendRoster {
    std::uint64_t identity_=0;
    int slot_=-1;
    std::vector<FriendRow> rows_;
    bool known_=false;
    static bool Eligible(const Header& h){
        return h.origin==Origin::Console&&h.state==SignIn::Online&&!h.guest&&
               h.identity!=0&&h.controller>=0&&h.controller<4;
    }
public:
    void Clear(){identity_=0;slot_=-1;rows_.clear();known_=false;}
    bool KnownFor(const Header& h)const{
        return known_&&Eligible(h)&&identity_==h.identity&&slot_==h.controller;
    }
    const std::vector<FriendRow>& Rows()const{return rows_;}
    void Observe(const Header& h){if(!KnownFor(h))Clear();}
    bool Set(const Header& before,const Header& after,const unsigned char* data,
             std::size_t bytes,std::size_t count){
        Clear();
        if(!Eligible(before)||!Eligible(after)||before.identity!=after.identity||
           before.controller!=after.controller||!ParseFriendRoster(data,bytes,count,rows_))return false;
        identity_=after.identity;slot_=after.controller;known_=true;return true;
    }
};
}
