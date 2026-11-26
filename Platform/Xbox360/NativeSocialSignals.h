#pragma once
#include "SocialProfile.h"
#include <array>
#include <cstdint>

namespace InfinitySocial {
// XAM version-zero Live/Friends notification IDs. These signals are global;
// no user index, payload, gamertag, message text or XUID is copied into history.
constexpr std::uint64_t SocialListenerMask=(1ull<<1)|(1ull<<2);
enum class Signal : unsigned { None, Connection, InvitationAccepted, Invitation,
    Messages, Voice, Presence, FriendsAdded, FriendsRemoved, FriendRequest, Count };
inline Signal SocialSignal(std::uint32_t id){
    switch(id){
        case 0x02000001u:return Signal::Connection;
        case 0x02000002u:return Signal::InvitationAccepted;
        case 0x82000004u:case 0x82000005u:return Signal::Invitation;
        case 0x82000006u:return Signal::Messages;
        case 0x02000009u:return Signal::Voice;
        case 0x0200000au:case 0x04000001u:return Signal::Presence;
        case 0x04000002u:return Signal::FriendsAdded;
        case 0x04000003u:return Signal::FriendsRemoved;
        case 0x84000004u:case 0x84000005u:case 0x84000006u:return Signal::FriendRequest;
        default:return Signal::None;
    }
}
inline const char* SignalLabel(Signal signal){
    switch(signal){
        case Signal::Connection:return "Evento Xbox: estado de conexao alterado";
        case Signal::InvitationAccepted:return "Evento Xbox: convite aceito no sistema";
        case Signal::Invitation:return "Evento Xbox: atividade de convites";
        case Signal::Messages:return "Evento Xbox: lista de mensagens alterada";
        case Signal::Voice:return "Evento Xbox: estado do chat de voz alterado";
        case Signal::Presence:return "Evento Xbox: presenca social alterada";
        case Signal::FriendsAdded:return "Evento Xbox: lista de amigos recebeu alteracao";
        case Signal::FriendsRemoved:return "Evento Xbox: lista de amigos recebeu remocao";
        case Signal::FriendRequest:return "Evento Xbox: atividade de solicitacoes de amizade";
        default:return nullptr;
    }
}
inline bool EligibleForSocialSignals(const Header& h){
    return h.origin==Origin::Console&&h.state==SignIn::Online&&!h.guest&&
           h.identity!=0&&h.controller>=0&&h.controller<4;
}
// Per-kind throttling, even when multiple native IDs map to the same signal.
// Suppress on clock rollback; a subsequent fresh sample may re-establish time.
class SignalGate {
    std::array<std::uint64_t,static_cast<unsigned>(Signal::Count)> last_{};
    std::array<bool,static_cast<unsigned>(Signal::Count)> seen_{};
public:
    const char* Observe(std::uint32_t id,std::uint64_t now){
        const auto kind=SocialSignal(id);
        if(kind==Signal::None)return nullptr;
        const auto i=static_cast<unsigned>(kind);
        if(seen_[i]){
            if(now<last_[i]){last_[i]=now;return nullptr;}
            if(now-last_[i]<60000)return nullptr;
        }
        seen_[i]=true;last_[i]=now;return SignalLabel(kind);
    }
};
}
