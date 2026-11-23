#include "GameScanner.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main() {
    const fs::path root=fs::temp_directory_path()/
        ("aurorax-scanner-"+std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const fs::path standard=root/"Games"/"Halo 3";
    const fs::path custom=root/"XBOX 360"/"Forza Horizon";
    fs::create_directories(standard);
    fs::create_directories(custom);
    std::ofstream(standard/"default.xex")<<"fixture";
    std::ofstream(custom/"default.xex")<<"fixture";
    const fs::path config=root/"aurorax-scanpaths.txt";
    {
        std::ofstream file(config);
        file<<"# AuroraX paths\n"
            <<custom.parent_path().string()<<"\n"
            <<custom.parent_path().string()<<"/\n"
            <<"../outside\n"
            <<"/tmp/../outside\n"
            <<"bad*folder\n";
    }
    std::vector<std::string> defaults(1,standard.parent_path().string());
    const std::vector<std::string> roots=
        GameScanner::LoadRoots(config.string(),defaults);
    assert(roots.size()==2);
    assert(roots[0]==defaults[0]);
    assert(roots[1]==custom.parent_path().string());
    const std::vector<Game> entries=GameScanner(roots).Scan();
    assert(entries.size()==2);
    assert(entries[0].name=="Forza Horizon");
    assert(entries[1].name=="Halo 3");

    // Empty config is a non-destructive fallback to built-in directories.
    fs::remove(config);
    const std::vector<std::string> fallback=
        GameScanner::LoadRoots(config.string(),defaults);
    assert(fallback.size()==1 && fallback[0]==defaults[0]);

    // Limit untrusted/oversized configuration input.
    {
        std::ofstream file(config);
        file<<std::string(8200,'x')<<"\n"<<custom.parent_path().string()<<"\n";
    }
    const std::vector<std::string> limited=
        GameScanner::LoadRoots(config.string(),defaults);
    assert(limited.size()==1);
    fs::remove_all(root);
}
