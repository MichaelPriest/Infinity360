#pragma once
#include "GameScanner.h"
#include "HomeCanvas.h"
#include "cloud360.h"

class ConsoleHome {
public:
    enum Key { Left, Right, Up, Down, Accept, Back, Favorite, Refresh };
    ConsoleHome(const std::string& dataDir, const std::vector<std::string>& gameRoots);
    void Load();
    std::string Press(Key key);  // Only confirmation on an installed title supplies a launch path.
    void Draw(HomeCanvas& canvas, const std::string& clock) const;
    void LaunchFailed();
private:
    enum Section { Library, Iptv, Apps, Emulators, Files, System, Recent, Favorites };
    enum Page { Home, Listing, Details };
    std::string dir_, notice_;
    std::vector<std::string> roots_, history_, favorites_, folders_;
    std::vector<Game> games_, tools_;
    std::vector<const Game*> visible_;
    std::vector<FolderEntry> files_;
    std::vector<CloudItem> channels_;
    std::vector<size_t> filtered_;
    CloudCatalog catalog_;
    CloudFavorites channelFavorites_;
    int focus_, row_, tab_;
    Section section_;
    Page page_;
    size_t Count() const;
    void Enter(Section section);
    void Rebuild();
    void Reload();
    bool SaveProfile() const;
    bool Starred(const Game& game) const;
    const Game* Installed(const std::string& path) const;
    void DrawHome(HomeCanvas& c) const;
    void DrawSection(HomeCanvas& c) const;
};
