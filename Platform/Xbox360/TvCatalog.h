#pragma once
#include "cloud360.h"
#include <string>
#include <vector>

enum TvCategory { TvAll, TvLive, TvFilm, TvSeries, TvAnime, TvCartoon, TvFavorites, TvSources, TvHistory, TvTabCount };
struct TvItem {
    std::string name, group, uri, channel;
    uint64_t id;
    TvCategory category;
};
struct TvEpisode {
    std::string show;
    unsigned season, episode;
};
bool tvEpisode(const std::string& title, TvEpisode& result);
bool tvEndsWith(const std::string& value, const std::string& ending);
std::string tvLower(std::string s);
std::string tvLocalMovie(const std::string& playlist, const std::string& uri);
class TvCatalog {
public:
    TvCatalog();
    bool Load(const std::string& path);
    const std::vector<TvItem>& Items() const { return items_; }
    bool Limited() const { return limited_; }
private:
    std::vector<TvItem> items_;
    size_t textBytes_;
    bool limited_;
    static int Append(const CloudItem* item, void* self);
};
