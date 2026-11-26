#pragma once
// Portable read-only social/profile view model. No XAM calls or network I/O.
#include <array>
#include <cstddef>
#include <string>
#include <vector>
#include <cstdint>

namespace InfinitySocial {
enum class SignIn { Unknown, SignedOut, Local, Online };
enum class Origin { Unavailable, Console, Test };
enum class Chat { Unavailable, LocalOnly, XboxService };
constexpr std::size_t GamerpicSide=64, GamerpicBytes=GamerpicSide*GamerpicSide*4;
inline bool ValidGamerpic(const std::vector<unsigned char>& rgba) {
    if(rgba.size()!=GamerpicBytes)return false;
    for(std::size_t i=3;i<rgba.size();i+=4)if(rgba[i])return true;
    return false;
}
// XAM tile provider returns linear ARGB; convert to the canvas RGBA format.
inline bool DecodeGamerpic(const unsigned char* argb,std::size_t bytes,std::vector<unsigned char>& rgba) {
    rgba.clear();
    if(!argb||bytes!=GamerpicBytes)return false;
    rgba.resize(bytes);
    for(std::size_t i=0;i<bytes;i+=4){
        rgba[i]=argb[i+1];rgba[i+1]=argb[i+2];rgba[i+2]=argb[i+3];rgba[i+3]=argb[i];
    }
    if(!ValidGamerpic(rgba)){rgba.clear();return false;}
    return true;
}

struct Slot {
    SignIn state = SignIn::Unknown;
    Origin origin = Origin::Unavailable;
    bool guest = false;
    std::string gamertag;
    std::string avatarAsset; // Provider-owned local asset; never an arbitrary URL.
    bool gamerscoreKnown = false;
    unsigned gamerscore = 0;
    std::vector<unsigned char> gamerpic;
    bool achievementsKnown=false;
    unsigned achievements=0;
    std::uint64_t identity=0; // RAM only; never serialize or render.
    std::vector<unsigned char> avatarPortrait; // 64x64 raster tile, not 3D.
};
struct Snapshot {
    std::array<Slot,4> slots{};
    Chat chat = Chat::Unavailable;
    bool serviceVerified = false;
};

inline bool IsSignedIn(const Slot& slot) {
    return slot.origin != Origin::Unavailable &&
           (slot.state == SignIn::Local || slot.state == SignIn::Online);
}
inline bool ValidGamertag(const std::string& value) {
    if (value.empty() || value.size() > 64) return false;
    bool visible = false;
    for (unsigned char c : value) {
        if (c < 32 || c == 127) return false;
        if (c != ' ') visible = true;
    }
    return visible;
}
inline bool ValidAvatarAsset(const std::string& value) {
    // Identifier relative to a provider's asset directory; never a path.
    if (value.empty() || value.size() > 64 || value == "." || value == "..") return false;
    for (unsigned char c : value) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-')) return false;
    }
    return true;
}
struct Header {
    int controller = -1;
    Origin origin = Origin::Unavailable;
    SignIn state = SignIn::Unknown;
    std::string displayName = "Perfil indisponivel";
    std::string avatarAsset;
    bool guest = false;
    bool gamerscoreKnown = false;
    unsigned gamerscore = 0;
    Chat chat = Chat::Unavailable;
    std::vector<unsigned char> gamerpic;
    bool achievementsKnown=false;
    unsigned achievements=0;
    std::uint64_t identity=0; // RAM only; never serialize or render.
    std::vector<unsigned char> avatarPortrait; // 64x64 raster tile, not 3D.
};

// Native system UI delegation is not a verified Xbox Live service.
enum class NativeAction { None, GamerCard, Friends, Achievements, Messages, QuickChat, AvatarEditor, RecentPlayers, EditProfile, FriendCard, ComposeMessage, Party };
constexpr int GuideSocialCount=6;
inline NativeAction GuideSocialAction(int row){
    switch(row){case 0:return NativeAction::GamerCard;case 1:return NativeAction::Friends;
        case 2:return NativeAction::Messages;case 3:return NativeAction::QuickChat;
        case 4:return NativeAction::AvatarEditor;case 5:return NativeAction::RecentPlayers;default:return NativeAction::None;}
}
inline int MoveGuideSocial(int row,int delta){
    if(row<0||row>=GuideSocialCount)row=0;
    return (row+GuideSocialCount+(delta<0?-1:1))%GuideSocialCount;
}
inline std::uint32_t NativeSocialOrdinal(NativeAction action){
    switch(action){case NativeAction::GamerCard:return 0x2c4u;case NativeAction::Friends:return 0x2bfu;
        case NativeAction::Achievements:return 0x2c5u;case NativeAction::Messages:return 0x2c0u;
        case NativeAction::QuickChat:return 0x2c2u;case NativeAction::AvatarEditor:return 0x7dbu;case NativeAction::RecentPlayers:return 0x2c8u;
        case NativeAction::EditProfile:return 0x3bcu;case NativeAction::FriendCard:return 0x2d5u;
        case NativeAction::ComposeMessage:return 0x2ccu;case NativeAction::Party:return 0x305u;default:return 0;}
}
// Communication hub routes to system-owned UI; never implies voice is connected.
constexpr int CommunicationCount=4;
inline NativeAction CommunicationAction(int row){
    switch(row){case 0:return NativeAction::Party;case 1:return NativeAction::QuickChat;
        case 2:return NativeAction::Friends;case 3:return NativeAction::Messages;default:return NativeAction::None;}
}
inline int MoveCommunication(int row,int delta){
    if(row<0||row>=CommunicationCount)row=0;
    return (row+CommunicationCount+(delta<0?-1:1))%CommunicationCount;
}
inline bool TargetedSocialAction(NativeAction action){
    return action==NativeAction::FriendCard||action==NativeAction::ComposeMessage;
}
inline bool CanRequestNative(NativeAction action,const Header& h,std::uint32_t title=0,std::uint64_t target=0) {
    if(h.origin!=Origin::Console || h.guest || h.controller<0 || h.controller>=4 ||
       !ValidGamertag(h.displayName)||h.identity==0)return false;
    if(TargetedSocialAction(action))return h.state==SignIn::Online&&target!=0&&target!=h.identity;
    if(target)return false;
    if(action==NativeAction::GamerCard||action==NativeAction::AvatarEditor||action==NativeAction::EditProfile)return h.state==SignIn::Local || h.state==SignIn::Online;
    if(action==NativeAction::Friends||action==NativeAction::Messages||action==NativeAction::QuickChat||action==NativeAction::RecentPlayers||action==NativeAction::Party)return h.state==SignIn::Online;
    if(action==NativeAction::Achievements)return title!=0&&(h.state==SignIn::Local||h.state==SignIn::Online);
    return false;
}

// Avatar editor may terminate the running title. Confirmation is tied to the
// current account, slot and sign-in state, and never persisted.
class AvatarExitConfirmation {
    std::uint64_t identity_=0;
    int slot_=-1;
    SignIn state_=SignIn::Unknown;
public:
    void Cancel(){identity_=0;slot_=-1;state_=SignIn::Unknown;}
    bool Matches(const Header& h)const{
        return identity_!=0&&identity_==h.identity&&slot_==h.controller&&state_==h.state&&
            CanRequestNative(NativeAction::AvatarEditor,h);
    }
    bool Accept(const Header& h){
        if(!CanRequestNative(NativeAction::AvatarEditor,h)){Cancel();return false;}
        if(Matches(h)){Cancel();return true;}
        identity_=h.identity;slot_=h.controller;state_=h.state;return false;
    }
};
inline bool SameArtAccount(const Header& a,const Header& b){
    return a.origin==Origin::Console&&b.origin==Origin::Console&&!a.guest&&!b.guest&&
        a.identity!=0&&a.identity==b.identity&&a.controller==b.controller&&
        a.state==b.state&&(b.state==SignIn::Local||b.state==SignIn::Online);
}
inline void RetainProfileArt(const Header& previous,Header& next,bool gamerpic,bool avatar){
    if(!SameArtAccount(previous,next))return;
    if(gamerpic&&ValidGamerpic(previous.gamerpic))next.gamerpic=previous.gamerpic;
    if(avatar&&ValidGamerpic(previous.avatarPortrait))next.avatarPortrait=previous.avatarPortrait;
}
// Compare every display-affecting field, not just name and sign-in state.
inline bool SameHeader(const Header& a, const Header& b) {
    return a.controller == b.controller && a.origin == b.origin &&
           a.state == b.state && a.displayName == b.displayName &&
           a.avatarAsset == b.avatarAsset && a.guest == b.guest &&
           a.gamerscoreKnown == b.gamerscoreKnown &&
           a.gamerscore == b.gamerscore && a.chat == b.chat && a.gamerpic==b.gamerpic && a.avatarPortrait==b.avatarPortrait && a.achievementsKnown==b.achievementsKnown && a.achievements==b.achievements && a.identity==b.identity;
}

// Confirm two consecutive console observations before emitting generic local
// session events. Unknown/test observations never imply sign-out or sign-in.
// Account identity is compared only in RAM; events never contain identifiers.
class SessionEvents {
    SignIn stable_=SignIn::Unknown, candidate_=SignIn::Unknown;
    int stableController_=-1, candidateController_=-1;
    std::uint64_t stableIdentity_=0,candidateIdentity_=0;
public:
    const char* Observe(const Header& h) {
        const bool signedIn=h.state==SignIn::Local || h.state==SignIn::Online;
        if(h.origin!=Origin::Console ||
           !(signedIn ? h.controller>=0 && h.controller<4 :
                         h.state==SignIn::SignedOut && h.controller==-1)) {
            candidate_=SignIn::Unknown;candidateController_=-1;candidateIdentity_=0;
            return nullptr;
        }
        const auto identity=signedIn?h.identity:0;
        if(candidate_!=h.state || candidateController_!=h.controller || candidateIdentity_!=identity) {
            candidate_=h.state;candidateController_=h.controller;candidateIdentity_=identity;
            return nullptr;
        }
        if(stable_==candidate_ && stableController_==candidateController_ && stableIdentity_==candidateIdentity_)return nullptr;
        const auto before=stable_;
        const int previousController=stableController_;
        const auto previousIdentity=stableIdentity_;
        stable_=candidate_;stableController_=candidateController_;stableIdentity_=candidateIdentity_;
        if(before==SignIn::Unknown)return nullptr;
        if(stable_==SignIn::SignedOut)return "Sessao Xbox: nenhum perfil ativo";
        if(before==SignIn::SignedOut)return "Sessao Xbox: perfil ativo detectado";
        if(previousController!=stableController_)return "Sessao Xbox: controle ativo alterado";
        if(previousIdentity&&stableIdentity_&&previousIdentity!=stableIdentity_)return "Sessao Xbox: conta ativa alterada";
        if(before==stable_)return nullptr;
        return stable_==SignIn::Online?"Sessao Xbox: estado online detectado":
                                        "Sessao Xbox: estado local detectado";
    }
};

// Confirm counter changes on two samples. Profile switches, unavailable data
// and state transitions reset baselines; decreases are silent corrections.
class CounterEvents {
    std::uint64_t identity_=0;
    int controller_=-1;
    SignIn state_=SignIn::Unknown;
    struct Counter {
        unsigned stable=0,candidate=0;
        bool ready=false,pending=false;
        bool Observe(bool known,unsigned value){
            if(!known){*this=Counter{};return false;}
            if(!pending||candidate!=value){candidate=value;pending=true;return false;}
            if(!ready){stable=value;ready=true;return false;}
            const bool increased=value>stable;stable=value;return increased;
        }
    } score_,achievements_;
public:
    const char* Observe(const Header& h){
        const bool eligible=h.origin==Origin::Console&&h.identity!=0&&!h.guest&&
            h.controller>=0&&h.controller<4&&(h.state==SignIn::Local||h.state==SignIn::Online);
        if(!eligible){*this=CounterEvents{};return nullptr;}
        if(h.identity!=identity_||h.controller!=controller_||h.state!=state_){
            *this=CounterEvents{};identity_=h.identity;controller_=h.controller;state_=h.state;
        }
        const bool score=score_.Observe(h.gamerscoreKnown,h.gamerscore);
        const bool earned=achievements_.Observe(h.achievementsKnown,h.achievements);
        return earned?"Perfil Xbox: aumento de conquistas reportado":
            score?"Perfil Xbox: aumento de Gamerscore reportado":nullptr;
    }
};
inline Header Resolve(const Snapshot& snapshot, int preferredController) {
    Header result;
    int selected = -1;
    if (preferredController >= 0 && preferredController < 4 &&
        IsSignedIn(snapshot.slots[static_cast<std::size_t>(preferredController)]))
        selected = preferredController;
    if (selected < 0) {
        for (int i = 0; i < 4; ++i) {
            if (IsSignedIn(snapshot.slots[static_cast<std::size_t>(i)])) {
                selected = i;
                break;
            }
        }
    }
    if (selected < 0) {
        // A partial or failed query cannot establish that every controller is signed out.
        bool knownSignedOut = true;
        for (const auto& slot : snapshot.slots) {
            if (slot.origin != Origin::Console || slot.state != SignIn::SignedOut)
                knownSignedOut = false;
        }
        if (knownSignedOut) {
            result.origin = Origin::Console;
            result.state = SignIn::SignedOut;
            result.displayName = "Nenhum perfil identificado";
        }
        return result;
    }
    const auto& slot = snapshot.slots[static_cast<std::size_t>(selected)];
    result.controller = selected;
    result.origin = slot.origin;
    result.state = slot.state;
    result.guest = slot.guest;
    if(slot.origin==Origin::Console)result.identity=slot.identity;
    result.displayName = ValidGamertag(slot.gamertag) ? slot.gamertag :
                         (slot.guest ? "Convidado" : "Perfil sem nome");
    if (ValidAvatarAsset(slot.avatarAsset)) result.avatarAsset = slot.avatarAsset;
    if(!slot.guest && ValidGamerpic(slot.gamerpic))result.gamerpic=slot.gamerpic;
    if(!slot.guest && ValidGamerpic(slot.avatarPortrait))result.avatarPortrait=slot.avatarPortrait;
    result.gamerscoreKnown = !slot.guest && slot.gamerscoreKnown;
    if (result.gamerscoreKnown) result.gamerscore = slot.gamerscore;
    result.achievementsKnown=!slot.guest&&slot.achievementsKnown;
    if(result.achievementsKnown)result.achievements=slot.achievements;
    if (snapshot.chat == Chat::LocalOnly) result.chat = Chat::LocalOnly;
    // A test/local/guest account never enables real Xbox communications.
    if (snapshot.chat == Chat::XboxService && snapshot.serviceVerified &&
        slot.origin == Origin::Console && slot.state == SignIn::Online && !slot.guest)
        result.chat = Chat::XboxService;
    return result;
}
} // namespace InfinitySocial
