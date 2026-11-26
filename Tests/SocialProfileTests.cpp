#include "SocialProfile.h"
#include <iostream>
using namespace InfinitySocial;
#define CHECK(expr) do { if (!(expr)) { std::cerr << "line " << __LINE__ << ": " #expr "\n"; return 1; } } while (false)
int main() {
    Snapshot s;
    CHECK(Resolve(s, 0).state == SignIn::Unknown);
    CHECK(Resolve(s, 0).chat == Chat::Unavailable);
    s.slots[0].origin = Origin::Console;
    s.slots[0].state = SignIn::SignedOut;
    CHECK(Resolve(s, 0).state == SignIn::SignedOut);
    s.slots[1] = {SignIn::Local, Origin::Console, false, "LocalPlayer", "avatar_1", true, 0};
    s.slots[2] = {SignIn::Online, Origin::Console, false, "OnlinePlayer", "avatar_2", true, 123};
    s.chat = Chat::XboxService;
    CHECK(Resolve(s, 0).controller == 1);
    CHECK(Resolve(s, -1).controller == 1);
    CHECK(Resolve(s, 5).controller == 1);
    CHECK(Resolve(s, 1).gamerscoreKnown && Resolve(s, 1).gamerscore == 0);
    CHECK(Resolve(s, 2).chat == Chat::Unavailable);
    s.serviceVerified = true;
    CHECK(Resolve(s, 2).chat == Chat::XboxService);
    CHECK(Resolve(s, 1).chat == Chat::Unavailable);
    s.slots[2].guest = true;
    CHECK(Resolve(s, 2).chat == Chat::Unavailable);
    CHECK(!Resolve(s, 2).gamerscoreKnown);
    s.slots[2].guest = false;
    s.slots[2].origin = Origin::Test;
    CHECK(Resolve(s, 2).chat == Chat::Unavailable);
    CHECK(Resolve(s, 2).origin == Origin::Test);
    s.slots[2].avatarAsset = "../unsafe";
    s.slots[2].gamertag = " ";
    CHECK(Resolve(s, 2).avatarAsset.empty());
    CHECK(Resolve(s, 2).displayName == "Perfil sem nome");
    s.chat = Chat::LocalOnly;
    CHECK(Resolve(s, 2).chat == Chat::LocalOnly);
    s.slots[1].state = SignIn::SignedOut;
    s.slots[2].state = SignIn::SignedOut;
    CHECK(Resolve(s, 2).controller == -1);
    CHECK(Resolve(s, 2).chat == Chat::Unavailable);
    std::cout << "social profile checks passed\n";
    return 0;
}
