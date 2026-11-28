#include "GameScanner.h"
#include "../Platform/Xbox360/ControllerNavigation.h"
#include "XsdkPaths.h"
#include "XexLaunchCheck.h"
#include "DiscSet.h"
#include "DownloadRange.h"
#include <sstream>

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

int main() {
    {
        using namespace ControllerNavigation;
        State n;
        assert(n.Poll(true,Up,0,0,0).edges==Up);
        assert(n.Poll(true,Up,0,0,379).edges==0);
        assert(n.Poll(true,Up,0,0,380).edges==Up);
        assert(n.Poll(true,Up,0,0,495).edges==Up);
        assert(n.Poll(true,0,0,20000,500).edges==0);
        assert(n.Poll(true,0,0,12000,510).held==Up);
        assert(n.Poll(true,0,0,10000,520).held==0);
        assert(n.Poll(true,0,18000,19000,530).edges==Up);
        assert(n.Poll(true,0,20000,19000,540).held==Up);
        assert(n.Poll(true,Left,25000,0,550).edges==Left);
        assert(n.Poll(true,Left|Right,25000,0,560).held==0);
        assert(n.Poll(true,Right|0x1000,0,0,570).edges==0x1000);
        assert(n.Poll(true,Right|0x1000,0,0,600).edges==0);
        n.Suppress();
        assert(n.Poll(true,Right,0,0,1000).edges==0);
        assert(n.Poll(true,0,0,0,1001).edges==0);
        assert(n.Poll(true,Right,0,0,1002,false).edges==Right);
        assert(n.Poll(true,Right,0,0,5000,false).edges==0);
        assert(n.Poll(false,0,0,0,5100).edges==0);
        assert(n.Poll(true,0,-25000,0,5200).edges==0);
        assert(n.Poll(true,0,0,0,5201).edges==0);
        assert(n.Poll(true,0,-32768,0,5202).edges==Left);
    }
    assert(DownloadRange::Matches("bytes 100-999/1000",100,1000));
    assert(!DownloadRange::Matches("bytes 0-999/1000",100,1000));
    assert(!DownloadRange::Matches("bytes 100-999/1001",100,1000));
    assert(!DownloadRange::Matches("bytes 100-998/1000",100,1000));
    assert(!DownloadRange::Matches("bytes 18446744073709551616-999/1000",100,1000));
    assert(!DownloadRange::Matches("bytes 100-999/1000junk",100,1000));
    assert(XexLaunchCheck::Path("HDD1:\\Games\\Test\\default.XEX")=="hdd:/Games/Test/default.XEX");
    assert(XexLaunchCheck::Path("game:/infinity_menu_preview.xex").empty());
    assert(XexLaunchCheck::Path("hdd:/Content/package").empty());
    assert(XexLaunchCheck::Path("flash:/default.xex").empty());
    std::string image(8192,0);
    image.replace(0,4,"XEX2");image[10]=16;image[19]=32;
    {std::istringstream stream(image);assert(XexLaunchCheck::Header(stream));}
    XexLaunchCheck::Ticket ticketA{},ticketB{};
    {std::istringstream stream(image);assert(XexLaunchCheck::Capture(stream,ticketA));}
    {std::istringstream stream(image);assert(XexLaunchCheck::Capture(stream,ticketB)&&XexLaunchCheck::Same(ticketA,ticketB));}
    image.back()='x';
    {std::istringstream stream(image);assert(XexLaunchCheck::Capture(stream,ticketB)&&!XexLaunchCheck::Same(ticketA,ticketB));}
    image.back()=0;
    assert(XexLaunchCheck::OwnTitle(0x494E4633u)&&!XexLaunchCheck::OwnTitle(0x494E4634u));
    XboxExecutionInfo disc1{},disc2{};disc1.titleId=disc2.titleId=0x494E1234;disc1.baseVersion=disc2.baseVersion=1;
    disc1.discCount=disc2.discCount=2;disc1.discNumber=1;disc2.discNumber=2;disc1.mediaId=7;disc2.mediaId=8;
    assert(DiscSet::Matches(disc1,disc2));disc2.baseVersion=2;assert(!DiscSet::Matches(disc1,disc2));
    disc2.baseVersion=1;disc2.discNumber=3;assert(!DiscSet::Matches(disc1,disc2));
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
    const auto depthFile=root/"depths.txt";
    {std::ofstream out(depthFile);out<<"0\t"<<roots[0]<<"\r\n1\t"<<roots[1]<<"\n99\t"<<roots[1]<<"\n";}
    const auto depths=GameScanner::LoadDepths(depthFile.string());
    assert(depths.size()==2&&GameScanner::DepthFor(depths,roots[0])==0&&GameScanner::DepthFor(depths,roots[1])==1);
    const auto shallow=GameScanner(roots).Scan(depths);
    assert(shallow.size()==1&&shallow[0].name=="Forza Horizon");
    assert(GameScanner::DepthFor({},roots[0])==16);
    {std::ofstream out(depthFile);out<<std::string(8193,'x');}
    assert(GameScanner::LoadDepths(depthFile.string()).empty());

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
