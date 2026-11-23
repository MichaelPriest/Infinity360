#include "ConsoleHome.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>

int main(int argc,char** argv) {
    assert(argc==2);
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/("aurorax-home-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root/"Games"/"Halo 3");
    const std::string exe=(root/"Games"/"Halo 3"/"default.xex").string();
    std::ofstream(exe)<<"fixture";
    const std::string dir=root.string()+"/";
    const std::vector<std::string> roots(1,(root/"Games").string());
    ConsoleHome menu(dir,roots);
    menu.Load();
    HomeCanvas canvas;
    assert(canvas.LoadArt(argv[1]));
    menu.Draw(canvas,"19:42");
    assert(canvas.Pixels()[200*1280+42]==0xff9dec31u);
    menu.Press(ConsoleHome::Accept);                 // Library
    menu.Press(ConsoleHome::Favorite);
    assert(menu.Press(ConsoleHome::Accept).empty()); // Details
    assert(menu.Press(ConsoleHome::Accept)==exe);
    ConsoleHome restored(dir,roots); restored.Load();
    restored.Press(ConsoleHome::Down); restored.Press(ConsoleHome::Down); // Recent
    restored.Press(ConsoleHome::Accept); restored.Press(ConsoleHome::Accept);
    assert(restored.Press(ConsoleHome::Accept)==exe);
    restored.Press(ConsoleHome::Back); restored.Press(ConsoleHome::Back);
    restored.Press(ConsoleHome::Right); restored.Press(ConsoleHome::Accept); // Favorites
    restored.Press(ConsoleHome::Accept); assert(restored.Press(ConsoleHome::Accept)==exe);
    restored.Press(ConsoleHome::Favorite);           // Remove last favorite in Details
    assert(restored.Press(ConsoleHome::Accept).empty());

    std::ofstream(dir+"playlist.m3u")<<"#EXTM3U\n#EXTINF:-1 group-title=\"Filmes\",Filme de teste\nhttps://example.invalid/film.mp4\n";
    menu.Press(ConsoleHome::Back); menu.Press(ConsoleHome::Back); menu.Press(ConsoleHome::Right);
    menu.Press(ConsoleHome::Accept);                 // Cloud360
    menu.Press(ConsoleHome::Right);                  // Live list is empty
    menu.Press(ConsoleHome::Favorite);
    assert(!fs::exists(root/"favorites.txt"));
    menu.Press(ConsoleHome::Right);                  // Movies
    menu.Press(ConsoleHome::Favorite);
    CloudFavorites fav = {};
    assert(cloud_favorites_load(&fav,(dir+"favorites.txt").c_str())==0 && fav.count==1);
    fs::remove(root/"playlist.m3u");
    menu.Press(ConsoleHome::Refresh); menu.Press(ConsoleHome::Favorite);
    assert(cloud_favorites_load(&fav,(dir+"favorites.txt").c_str())==0 && fav.count==1);
    menu.Draw(canvas,"19:43");                       // Render missing-playlist state
    fs::remove_all(root);
}
