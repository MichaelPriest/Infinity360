#pragma once
// Portable read-only social/profile view model. No XAM calls or network I/O.
#include <array>
#include <cstddef>
#include <string>

namespace InfinitySocial {
enum class SignIn { Unknown, SignedOut, Local, Online };
enum class Origin { Unavailable, Console, Test };
enum class Chat { Unavailable, LocalOnly, XboxService };

struct Slot {
    SignIn state = SignIn::Unknown;
    Origin origin = Origin::Unavailable;
    bool guest = false;
    std::string gamertag;
    std::string avatarAsset; // Provider-owned local asset; never an arbitrary URL.
    bool gamerscoreKnown = false;
    unsigned gamerscore = 0;
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
};

// Native system UI delegation is not a verified Xbox Live service.
enum class NativeAction { None, GamerCard, Friends };
inline bool CanRequestNative(NativeAction action,const Header& h) {
    if(h.origin!=Origin::Console || h.guest || h.controller<0 || h.controller>=4 ||
       !ValidGamertag(h.displayName))return false;
    if(action==NativeAction::GamerCard)return h.state==SignIn::Local || h.state==SignIn::Online;
    if(action==NativeAction::Friends)return h.state==SignIn::Online;
    return false;
}

// Compare every display-affecting field, not just name and sign-in state.
inline bool SameHeader(const Header& a, const Header& b) {
    return a.controller == b.controller && a.origin == b.origin &&
           a.state == b.state && a.displayName == b.displayName &&
           a.avatarAsset == b.avatarAsset && a.guest == b.guest &&
           a.gamerscoreKnown == b.gamerscoreKnown &&
           a.gamerscore == b.gamerscore && a.chat == b.chat;
}

// Confirm two consecutive console observations before emitting generic local
// session events. Unknown/test observations never imply sign-out or sign-in.
// Store only state and controller, never account names or identifiers.
class SessionEvents {
    SignIn stable_=SignIn::Unknown, candidate_=SignIn::Unknown;
    int stableController_=-1, candidateController_=-1;
public:
    const char* Observe(const Header& h) {
        const bool signedIn=h.state==SignIn::Local || h.state==SignIn::Online;
        if(h.origin!=Origin::Console ||
           !(signedIn ? h.controller>=0 && h.controller<4 :
                         h.state==SignIn::SignedOut && h.controller==-1)) {
            candidate_=SignIn::Unknown;candidateController_=-1;
            return nullptr;
        }
        if(candidate_!=h.state || candidateController_!=h.controller) {
            candidate_=h.state;candidateController_=h.controller;
            return nullptr;
        }
        if(stable_==candidate_ && stableController_==candidateController_)return nullptr;
        const auto before=stable_;
        const int previousController=stableController_;
        stable_=candidate_;stableController_=candidateController_;
        if(before==SignIn::Unknown)return nullptr;
        if(stable_==SignIn::SignedOut)return "Sessao Xbox: nenhum perfil ativo";
        if(before==SignIn::SignedOut)return "Sessao Xbox: perfil ativo detectado";
        if(previousController!=stableController_)return "Sessao Xbox: controle ativo alterado";
        return stable_==SignIn::Online?"Sessao Xbox: estado online detectado":
                                        "Sessao Xbox: estado local detectado";
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
    result.displayName = ValidGamertag(slot.gamertag) ? slot.gamertag :
                         (slot.guest ? "Convidado" : "Perfil sem nome");
    if (ValidAvatarAsset(slot.avatarAsset)) result.avatarAsset = slot.avatarAsset;
    result.gamerscoreKnown = !slot.guest && slot.gamerscoreKnown;
    if (result.gamerscoreKnown) result.gamerscore = slot.gamerscore;
    if (snapshot.chat == Chat::LocalOnly) result.chat = Chat::LocalOnly;
    // A test/local/guest account never enables real Xbox communications.
    if (snapshot.chat == Chat::XboxService && snapshot.serviceVerified &&
        slot.origin == Origin::Console && slot.state == SignIn::Online && !slot.guest)
        result.chat = Chat::XboxService;
    return result;
}
} // namespace InfinitySocial
