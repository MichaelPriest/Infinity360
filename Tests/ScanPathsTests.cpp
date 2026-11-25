#include "GameScanner.h"
#include "XsdkPaths.h"
#include "XexLaunchCheck.h"
#include <sstream>

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main() {
    assert(XexLaunchCheck::Path("HDD1:\\Games\\Test\\default.XEX")=="hdd:/Games/Test/default.XEX");
    assert(XexLaunchCheck::Path("game:/infinity_menu_preview.xex").empty());
    assert(XexLaunchCheck::Path("hdd:/Content/package").empty());
    assert(XexLaunchCheck::Path("flash:/default.xex").empty());
    std::string image(8192,0);
    image.replace(0,4,"XEX2");image[10]=16;image[19]=32;
    {std::istringstream stream(image);assert(XexLaunchCheck::Header(stream));}
    {std::istringstream stream("XEX2");assert(!XexLaunchCheck::Header(stream));}
    image[10]=64;
    {std::istringstream stream(image);assert(!XexLaunchCheck::Header(stream));}
    image[10]=16;image[19]=8;
    {std::istringstream stream(image);assert(!XexLaunchCheck::Header(stream));}
    assert(XsdkPaths::Normalize("HDD1:\\Games\\Halo")=="hdd:/Games/Halo");
    assert(XsdkPaths::Normalize("USB:/Games/")=="usb0:/Games");
    assert(XsdkPaths::Key("MU:/SAVE")==XsdkPaths::Key("mu0:/save"));
    assert(XsdkPaths::Normalize("flash:/system").empty());
    assert(XsdkPaths::Normalize("hdd:/Games/../secret").empty());
    assert(XsdkPaths::Normalize("hdd:/Games/usb1:/secret").empty());
    assert(XsdkPaths::Normalize("hdd:/").empty());
    assert(XsdkPaths::Inside("usb2:/Games","usb2:/"));
    assert(!XsdkPaths::Inside("game:/MenuPreviewOther","game:/MenuPreview"));
    assert(!XsdkPaths::Inside("game:/MenuPreview/hdd:/Games","game:/MenuPreview"));
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
    std::vector<std::string> overlapping=roots;
    overlapping.push_back(standard.string());
    assert(GameScanner(overlapping).Scan().size()==2);
    assert(GameScanner::ValidRoot("../bad").empty());
    fs::path deep=custom.parent_path();
    for(int i=0;i<17;++i)deep/="d";
    fs::create_directories(deep);
    std::ofstream(deep/"default.xex")<<"too deep";
    assert(GameScanner(roots).Scan().size()==2);
#ifndef _WIN32
    std::error_code linkError;
    fs::create_directory_symlink(custom.parent_path(),standard/"cycle",linkError);
    if(!linkError)assert(GameScanner(roots).Scan().size()==2);
#endif
    fs::remove_all(root);
}
