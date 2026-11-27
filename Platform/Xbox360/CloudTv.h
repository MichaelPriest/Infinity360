#pragma once
#include "TvCatalog.h"
#include "TvGuide.h"
#include "HomeCanvas.h"
#include "TvPreferences.h"

enum MenuKey { MenuLeft, MenuRight, MenuUp, MenuDown, MenuAccept, MenuBack, MenuFavorite,
               MenuRefresh, MenuPageUp, MenuPageDown, MenuGuide, MenuSearch, MenuGroups, MenuClearHistory, MenuSettings };
class CloudTv {
public:
    explicit CloudTv(const std::string& dir);
    void Open();
    bool Press(MenuKey key); // false returns to the dashboard
    void Draw(HomeCanvas& canvas,int64_t now,bool modern=false) const;
    std::string TakeMovie();
    bool CanZap()const;
    void RecordPlayed();
    TvPlaybackInfo PlayingInfo(int64_t now)const;
    void TogglePlayingFavorite();
    void FinishPlayback();
    std::vector<TvChannelChoice> ChannelChoices()const;
    std::string ChooseChannel(size_t row);
    std::vector<unsigned char> PlayingLogo()const;
    std::string Zap(int direction); // -1 previous, +1 next, 0 last selected
    void PlayerMessage(const std::string& text) { notice_=text; }
    size_t Count() const { return catalog_.Items().size(); }
    bool CanSearch()const{return view_==Browse&&tab_!=TvSources;}
    bool RemoteAllowed(MenuKey key)const{
        if(view_==Settings||view_==ClearHistory)return false;
        if(key==MenuFavorite&&tab_==TvSources)return false;
        return key==MenuLeft||key==MenuRight||key==MenuUp||key==MenuDown||key==MenuAccept||key==MenuBack||key==MenuFavorite||key==MenuRefresh||key==MenuPageUp||key==MenuPageDown||key==MenuGuide;
    }
    bool CanSettings()const{return view_==Browse;}
    bool CanClearHistory()const{return view_==Browse&&tab_==TvHistory&&!history_.empty();}
private:
    enum View { Browse, Guide, Details, Episodes, Search, Groups, ClearHistory, Settings };
    std::string dir_,source_,notice_,pending_,show_,query_,edit_;
    std::vector<std::string> sources_;
    std::string groupFilter_;
    struct GroupRow {std::string name;size_t count;};
    std::vector<GroupRow> groups_;
    size_t groupRow_=0,settingsRow_=0;
    void DrawSettings(HomeCanvas& c)const;
    void SettingsKey(MenuKey key);
    void OpenGroups();
    void DrawGroups(HomeCanvas& c)const;
    bool SaveHistory()const;
    std::vector<size_t> rows_,episodes_;
    std::vector<unsigned> seasons_;
    TvCatalog catalog_;
    TvGuide guide_;
    CloudFavorites favorites_;
    std::vector<uint64_t> history_;
    bool historyDirty_=false;
    void LoadHistory();
    size_t row_,episodeRow_,season_,detail_;
    size_t lastLive_=static_cast<size_t>(-1),activeLive_=static_cast<size_t>(-1);
    TvCategory tab_;
    View view_,behindDetails_,behindEpisodes_;
    int guideOffset_,letter_;
    void Discover();
    void SearchKey(MenuKey key);
    void DrawSearch(HomeCanvas& c) const;
    bool Load(const std::string& source);
    void Filter();
    void CollectEpisodes();
    void OpenItem(size_t index);
    void Toggle(size_t index,bool refresh=true);
    void DrawArt(HomeCanvas& c,size_t index,int x,int y,int w,int h) const;
    void DrawList(HomeCanvas& c,int64_t now) const;
    void DrawModernList(HomeCanvas& c,int64_t now) const;
    void DrawGuide(HomeCanvas& c,int64_t now) const;
    void DrawDetails(HomeCanvas& c,int64_t now) const;
    void DrawEpisodes(HomeCanvas& c) const;
};
