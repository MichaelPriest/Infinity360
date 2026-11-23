#pragma once
#include "GameScanner.h"
#include "HomeCanvas.h"
#include "CloudTv.h"
#include "CollectionStore.h"

class ConsoleHome {
public:
    enum Key { Left=MenuLeft, Right=MenuRight, Up=MenuUp, Down=MenuDown, Accept=MenuAccept, Back=MenuBack,
               Favorite=MenuFavorite, Refresh=MenuRefresh, PageUp=MenuPageUp, PageDown=MenuPageDown, Guide=MenuGuide, Search=MenuSearch };
    ConsoleHome(const std::string& dataDir, const std::vector<std::string>& gameRoots);
    void Load();
    std::string Press(Key key);  // Only confirmation on an installed title supplies a launch path.
    void Draw(HomeCanvas& canvas, const std::string& clock, int64_t now=-1) const;
    std::string TakeMovie() { return tv_.TakeMovie(); }
    void PlayerMessage(const std::string& text) { tv_.PlayerMessage(text); }
    void LaunchFailed();
private:
    enum Section { Library, Iptv, Apps, Emulators, Files, System, Recent, Favorites };
    enum Page { Home, Listing, Details };
    std::string dir_, notice_;
    std::vector<std::string> roots_, baseRoots_, history_, favorites_, hidden_, launched_, folders_;
    std::vector<Game> games_, tools_;
    std::vector<int> toolKinds_; // 1 = installed, 2 = plugin, 3 = homebrew
    std::vector<const Game*> visible_;
    std::vector<FolderEntry> files_;
    CloudTv tv_;
    CollectionStore collections_;
    int focus_, row_, subtab_, systemTab_, gameSort_, searchCursor_;
    int collectionIndex_, collectionRow_, keyboardMode_; // 0 search, 1 create, 2 rename
    bool searchActive_, collectionPicker_, deleteCollectionConfirm_;
    std::string searchTerm_, searchDraft_, pickerGame_;
    void ApplySearch();
    Section section_;
    Page page_;
    size_t Count() const;
    void Enter(Section section);
    void Rebuild();
    void Reload();
    void ScanLibrary();
    bool SaveProfile() const;
    bool Starred(const Game& game) const;
    const Game* Installed(const std::string& path) const;
    void DrawHome(HomeCanvas& c) const;
    void DrawSection(HomeCanvas& c) const;
};
