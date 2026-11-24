#include "ConsoleHome.h"
#include "MediaUrl.h"
#include <cassert>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iterator>

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
    {std::ifstream profile(root/"aurorax-profile.txt");
     const std::string data((std::istreambuf_iterator<char>(profile)),
                            std::istreambuf_iterator<char>());
     assert(data.find("R\t")==std::string::npos); // Not recently played yet
    }
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
    // Both URL-only categories and catalogues beyond the legacy fixed buffer are common.
    fs::create_directories(root/"Listas");
    const auto sub=root/"Listas";
    const auto cinema=sub/"Cinema.m3u";
    {
        std::ofstream list(cinema);
        for(int i=0;i<1650;++i)
            list<<"#EXTINF:-1 group-title=\"Filmes\",Filme "<<i<<", Parte 2\nhttps://example.invalid/catalog/"<<i<<"\n";
        list<<"#EXTINF:-1 group-title=\"Animes\",Órbita\nhttps://example.invalid/catalog/a\n"
            <<"#EXTINF:-1 group-title=\"Desenhos\",Lume\nhttps://example.invalid/catalog/c\n"
            <<"#EXTINF:-1 group-title=\"Séries\",Estrada S02E01\nvideo/segundo.wmv\n"
            <<"#EXTINF:-1 group-title=\"Séries\",Estrada S01E02\nvideo/dois.wmv\n"
            <<"#EXTINF:-1 group-title=\"Séries\",Estrada S01E01\nvideo/um.wmv\n";
    }
    TvCatalog catalogue;assert(catalogue.Load(cinema.string()));
    assert(catalogue.Items().size()==1655 && !catalogue.Limited());
    assert(catalogue.Items()[0].name=="Filme 0, Parte 2");
    CloudItem oldItem={};std::strcpy(oldItem.name,"Parte 2");std::strcpy(oldItem.url,"https://example.invalid/catalog/0");
    assert(catalogue.Items()[0].id==cloud_item_id(&oldItem));
    assert(catalogue.Items()[1650].category==TvAnime && catalogue.Items()[1651].category==TvCartoon);
    assert(!catalogue.Load((sub/"gone.m3u").string()) && catalogue.Items().size()==1655);
    TvEpisode ep;assert(tvEpisode("Céu - 2x07 - Ilha",ep) && ep.show=="Céu" && ep.season==2 && ep.episode==7);
    assert(!tvEpisode("Céu 1920x1080",ep));assert(!tvEpisode("Noite Solar",ep));
    assert(tvLocalMovie("game:\\Listas\\Cinema.m3u","clips/01.WMV")=="game:\\Listas\\clips\\01.WMV");
    assert(tvLocalMovie("game:\\list.m3u","USB0:/TV/Canal.wmv")=="USB0:\\TV\\Canal.wmv");
    assert(tvLocalMovie("game:\\list.m3u","https://example.invalid/clip.wmv").empty());
    assert(tvLocalMovie("game:\\list.m3u","arquivo.xex").empty());

    MediaUrl url;
    assert(url.Parse("HTTPS://example.invalid:8443/channel?sig=a%2Bb#position"));
    assert(url.secure && url.host=="example.invalid" && url.port==8443 && url.target=="/channel?sig=a%2Bb");
    assert(!url.NeedsConverter());
    assert(url.Parse("http://192.168.1.3:8090/live.mpg") && !url.NeedsConverter());
    assert(url.Parse("https://example.invalid/live.M3U8?token=x") && url.NeedsConverter());
    assert(!url.Parse("brazucaplay:opaque") && !url.Parse("http://host:99999/file"));
    assert(!url.Parse("http://host/path\r\nheader") && !url.Parse("http://user:pass@host/"));

    const std::string xmlPath=(root/"guide.xml").string();
    std::ofstream(xmlPath)<<"<tv><channel id='tv.a'><display-name>Rota</display-name></channel><programme channel='tv.a' start='20261123200000 -0300' stop='20261123203000 -0300'>"
        "<title>Depois</title></programme><programme channel='tv.a' start='20261123193000 -0300'>"
        "<title>Rota &amp; Serra</title></programme><programme channel='other' start='20260230190000 +0000' "
        "stop='20260301193000 +0000'><title>Invalid date</title></programme></tv>";
    TvGuide epg;assert(epg.Load(xmlPath)==TvGuide::Ready);
    const auto& schedule=epg.For("tv.a","");assert(schedule.size()==2 && schedule[0].stop==schedule[1].start);
    const int64_t clock=1795473720; // 2026-11-23T22:42:00Z
    assert(TvGuide::Current(schedule,clock) && TvGuide::Current(schedule,clock)->title=="Rota & Serra");
    assert(TvGuide::Next(schedule,clock)->title=="Depois");
    assert(!TvGuide::Current(schedule,schedule[1].stop));
    assert(epg.For("other","").empty());assert(epg.For("","tv.a").size()==2);assert(epg.For("","Rota").size()==2);
    std::ofstream(xmlPath)<<"<tv><programme></tv>";assert(epg.Load(xmlPath)==TvGuide::Invalid);
    assert(epg.For("tv.a","").empty());assert(epg.Load(dir+"ausente.xml")==TvGuide::Absent);

    CloudTv tv(dir);tv.Open();assert(tv.Count()==1655); // finds Listas/Cinema.m3u
    tv.Press(MenuRight);tv.Press(MenuRight);tv.Press(MenuRight); // Séries
    tv.Press(MenuAccept);tv.Press(MenuAccept);tv.Press(MenuAccept); // Temporada 1, primeiro episódio
    assert(tv.TakeMovie()==(sub/"video/um.wmv").string());assert(tv.TakeMovie().empty());
    tv.Press(MenuBack);tv.Press(MenuRight);tv.Press(MenuAccept);tv.Press(MenuAccept);
    assert(tv.TakeMovie()==(sub/"video/segundo.wmv").string());tv.Press(MenuBack);tv.Press(MenuBack);
    tv.Press(MenuRight); // Animes
    tv.Press(MenuSearch); // Search O without typing its accent
    tv.Press(MenuDown);tv.Press(MenuDown);tv.Press(MenuRight);tv.Press(MenuRight);tv.Press(MenuAccept);
    tv.Press(MenuRefresh);tv.Press(MenuFavorite);
    CloudFavorites f2={};assert(cloud_favorites_load(&f2,(dir+"favorites.txt").c_str())==0);
    assert(cloud_favorite_has_id(&f2,catalogue.Items()[1650].id));
    tv.Press(MenuAccept);tv.Press(MenuAccept);assert(tv.TakeMovie()=="https://example.invalid/catalog/a");tv.Press(MenuBack);
    tv.Press(MenuGuide);tv.Press(MenuRight);tv.Draw(canvas,clock);tv.Press(MenuBack);
    assert(!tv.Press(MenuBack));

    std::ofstream(root/"playlist.m3u")<<"#EXTM3U\n#EXTINF:-1 group-title=\"TV ao vivo\",Teste local\nclip.wmv\n";
    CloudTv source(dir);source.Open();assert(source.Count()==1);
    source.Press(MenuLeft); // Fontes, selecionar Cinema na subpasta
    source.Press(MenuAccept);
    assert(source.Count()==1655);CloudTv again(dir);again.Open();assert(again.Count()==1655);

    // A WMV request passes through the real dashboard without entering the XEX launch channel.
    fs::remove(root/"cloud360-source.txt");
    ConsoleHome mediaMenu(dir,roots);mediaMenu.Load();mediaMenu.Press(ConsoleHome::Right);
    mediaMenu.Press(ConsoleHome::Accept);mediaMenu.Press(ConsoleHome::Accept);
    assert(mediaMenu.Press(ConsoleHome::Accept).empty());assert(mediaMenu.TakeMovie()==dir+"clip.wmv");
    std::ofstream(root/"playlist.m3u")<<"#EXTM3U\n#EXTINF:-1,Canal na rede\nhttp://192.168.1.10:8090/channel/id.mpg\n"
        "#EXTINF:-1,HLS original\nhttps://example.invalid/live.m3u8\n#EXTINF:-1,Nexus\nbrazucaplay:sample\n";
    ConsoleHome streamMenu(dir,roots);streamMenu.Load();streamMenu.Press(ConsoleHome::Right);
    streamMenu.Press(ConsoleHome::Accept);streamMenu.Press(ConsoleHome::Accept);
    assert(streamMenu.Press(ConsoleHome::Accept).empty());
    assert(streamMenu.TakeMovie()=="http://192.168.1.10:8090/channel/id.mpg" && streamMenu.TakeMovie().empty());
    for(int i=0;i<2;++i) {
        streamMenu.Press(ConsoleHome::Back);streamMenu.Press(ConsoleHome::Down);
        streamMenu.Press(ConsoleHome::Accept);assert(streamMenu.Press(ConsoleHome::Accept).empty());
        assert(streamMenu.TakeMovie().empty());
    }

    // Optional local XEX plugins extend Apps and Emulators without changing Home.
    const fs::path music=root/"Plugins"/"Music";
    fs::create_directories(music);
    std::ofstream(music/"default.xex")<<"test plugin";
    std::ofstream(music/"plugin.axplugin")
        <<"schema=1\n"<<"id=aurorax.music\n"<<"name=Music Player\n"
        <<"version=0.1\n"<<"category=app\n"
        <<"entry=default.xex\n"<<"enabled=true\n";
    const fs::path emulator=root/"Plugins"/"Retro";
    fs::create_directories(emulator);
    std::ofstream(emulator/"default.xex")<<"test plugin";
    std::ofstream(emulator/"plugin.axplugin")
        <<"schema=1\n"<<"id=aurorax.retro\n"<<"name=Retro Plugin\n"
        <<"version=0.1\n"<<"category=emulator\n"
        <<"entry=default.xex\n"<<"enabled=true\n";
    ConsoleHome pluginMenu(dir,roots); pluginMenu.Load();
    pluginMenu.Press(ConsoleHome::Down);   // Apps at home position 2
    pluginMenu.Press(ConsoleHome::Accept); // Apps listing
    pluginMenu.Press(ConsoleHome::Accept); // App details
    assert(pluginMenu.Press(ConsoleHome::Accept)==(music/"default.xex").string());
    pluginMenu.Press(ConsoleHome::Back);
    pluginMenu.Press(ConsoleHome::Back);
    pluginMenu.Press(ConsoleHome::Right);  // Emulators at home position 3
    pluginMenu.Press(ConsoleHome::Accept);
    pluginMenu.Press(ConsoleHome::Accept);
    assert(pluginMenu.Press(ConsoleHome::Accept)==(emulator/"default.xex").string());

    // New Apps -> Homebrew tab uses real local XEX discovery.
    const fs::path brew=root/"Homebrew"/"RetroArch 360";
    fs::create_directories(brew);
    std::ofstream(brew/"default.xex")<<"homebrew fixture";
    ConsoleHome tabsMenu(dir,roots); tabsMenu.Load();
    tabsMenu.Press(ConsoleHome::Down);             // Apps
    tabsMenu.Press(ConsoleHome::Accept);
    tabsMenu.Press(ConsoleHome::Right);            // Instalados (empty)
    assert(tabsMenu.Press(ConsoleHome::Accept).empty());
    tabsMenu.Press(ConsoleHome::Right);            // Plugins
    tabsMenu.Press(ConsoleHome::Accept);
    assert(tabsMenu.Press(ConsoleHome::Accept)==(music/"default.xex").string());
    tabsMenu.Press(ConsoleHome::Back);             // Return to tabbed listing
    tabsMenu.Press(ConsoleHome::Right);            // Homebrew
    tabsMenu.Press(ConsoleHome::Accept);
    assert(tabsMenu.Press(ConsoleHome::Accept)==(brew/"default.xex").string());
    tabsMenu.Press(ConsoleHome::Back);
    tabsMenu.Press(ConsoleHome::Left);             // Plugins remains selectable
    tabsMenu.Press(ConsoleHome::Accept);
    assert(tabsMenu.Press(ConsoleHome::Accept)==(music/"default.xex").string());
    tabsMenu.Press(ConsoleHome::Back);
    tabsMenu.Draw(canvas,"19:42");                // Render Homebrew/Plugins tab UI

    // System panel keeps the AuroraX theme, and clearly marks providers
    // which are not yet supported on physical Xbox 360 hardware.
    ConsoleHome sysMenu(dir,roots); sysMenu.Load();
    sysMenu.Press(ConsoleHome::Right);  // Cloud360
    sysMenu.Press(ConsoleHome::Down);   // Arquivos
    sysMenu.Press(ConsoleHome::Right);  // Sistema
    sysMenu.Press(ConsoleHome::Accept);
    sysMenu.Draw(canvas,"19:45");
    std::vector<unsigned> storageScreen(canvas.Pixels(),
        canvas.Pixels()+HomeCanvas::Width*HomeCanvas::Height);
    sysMenu.Press(ConsoleHome::Right);  // Rede e FTP card
    sysMenu.Draw(canvas,"19:45");
    assert(!std::equal(storageScreen.begin(),storageScreen.end(),canvas.Pixels()));
    sysMenu.Press(ConsoleHome::Accept);
    sysMenu.Draw(canvas,"19:45");
    sysMenu.Press(ConsoleHome::Back);  // System listing
    sysMenu.Press(ConsoleHome::Back);  // Home

    // Series360 parity: scan additional paths configured without rewriting
    // or deleting the console's existing game/library data.
    const fs::path extraDir=root/"XBOX 360"/"Forza Horizon";
    fs::create_directories(extraDir);
    std::ofstream(extraDir/"default.xex")<<"extra test";
    const fs::path scanConfig=root/"aurorax-scanpaths.txt";
    std::ofstream(scanConfig)<<extraDir.parent_path().string()<<"\n";
    ConsoleHome extraMenu(dir,roots); extraMenu.Load();
    extraMenu.Press(ConsoleHome::Accept); // Library, now includes configured path
    extraMenu.Press(ConsoleHome::Accept); // First game alphabetically
    assert(extraMenu.Press(ConsoleHome::Accept)==(extraDir/"default.xex").string());
    extraMenu.Press(ConsoleHome::Back);   // Listing
    fs::remove(scanConfig);
    extraMenu.Press(ConsoleHome::Refresh); // Re-read configured roots
    extraMenu.Press(ConsoleHome::Accept);
    assert(extraMenu.Press(ConsoleHome::Accept)==exe);

    // Series360 categories: only extracted XEX entries are indexed here.
    const fs::path arcade=root/"XBLA"/"Geometry Wars 2";
    const fs::path original=root/"Xbox Original"/"Fable";
    fs::create_directories(arcade);
    fs::create_directories(original);
    std::ofstream(arcade/"default.xex")<<"arcade fixture";
    std::ofstream(original/"default.xex")<<"original fixture";
    std::ofstream(scanConfig)
        <<extraDir.parent_path().string()<<"\n"
        <<arcade.parent_path().string()<<"\n"
        <<original.parent_path().string()<<"\n";
    ConsoleHome categoryMenu(dir,roots); categoryMenu.Load();
    categoryMenu.Press(ConsoleHome::Accept);  // Library
    categoryMenu.Press(ConsoleHome::Right);   // Xbox 360 (Forza + Halo)
    categoryMenu.Press(ConsoleHome::Accept);
    assert(categoryMenu.Press(ConsoleHome::Accept)==(extraDir/"default.xex").string());
    categoryMenu.Press(ConsoleHome::Back);
    categoryMenu.Press(ConsoleHome::Right);   // XBLA
    categoryMenu.Press(ConsoleHome::Accept);
    assert(categoryMenu.Press(ConsoleHome::Accept)==(arcade/"default.xex").string());
    categoryMenu.Press(ConsoleHome::Back);
    categoryMenu.Press(ConsoleHome::Right);   // Xbox Original
    categoryMenu.Press(ConsoleHome::Accept);
    assert(categoryMenu.Press(ConsoleHome::Accept)==(original/"default.xex").string());
    categoryMenu.Draw(canvas,"19:45");

    // Series360 RS Ordenar: stable native sort, no web mockup required.
    ConsoleHome orderedMenu(dir,roots); orderedMenu.Load();
    orderedMenu.Press(ConsoleHome::Accept); // Library: A-Z
    orderedMenu.Press(ConsoleHome::Search); // RS -> Z-A
    orderedMenu.Press(ConsoleHome::Accept);
    assert(orderedMenu.Press(ConsoleHome::Accept)==exe); // Halo 3 sorts first
    orderedMenu.Press(ConsoleHome::Back);
    orderedMenu.Press(ConsoleHome::Search); // RS -> device ordering
    orderedMenu.Draw(canvas,"19:45");

    // Library search: Start opens on-screen keyboard, A types, Y applies.
    ConsoleHome searchMenu(dir,roots); searchMenu.Load();
    searchMenu.Press(ConsoleHome::Accept); // Library / Todos
    searchMenu.Press(ConsoleHome::Guide);  // Start: keyboard
    for(int i=0;i<6;++i) searchMenu.Press(ConsoleHome::Right);
    searchMenu.Press(ConsoleHome::Accept); // letter G
    searchMenu.Press(ConsoleHome::Refresh); // Y: apply
    searchMenu.Draw(canvas,"19:46");
    searchMenu.Press(ConsoleHome::Accept);
    assert(searchMenu.Press(ConsoleHome::Accept)==
           (arcade/"default.xex").string());
    searchMenu.Press(ConsoleHome::Back);
    searchMenu.Press(ConsoleHome::Guide);
    searchMenu.Press(ConsoleHome::Favorite); // X: remove G
    searchMenu.Press(ConsoleHome::Refresh);  // clear applied search
    searchMenu.Press(ConsoleHome::Accept);
    assert(searchMenu.Press(ConsoleHome::Accept)==
           (original/"default.xex").string());

    // Persisted hidden games never show in default tabs; Ocultos allows unhide.
    ConsoleHome hideMenu(dir,roots); hideMenu.Load();
    hideMenu.Press(ConsoleHome::Accept);
    hideMenu.Press(ConsoleHome::PageUp); // LB: hide first game (Fable)
    hideMenu.Press(ConsoleHome::Accept);
    assert(hideMenu.Press(ConsoleHome::Accept)==
           (extraDir/"default.xex").string());
    hideMenu.Press(ConsoleHome::Back);
    for(int i=0;i<7;++i) hideMenu.Press(ConsoleHome::Right); // Ocultos tab
    hideMenu.Press(ConsoleHome::Accept);
    assert(hideMenu.Press(ConsoleHome::Accept)==
           (original/"default.xex").string());
    hideMenu.Press(ConsoleHome::Back);
    hideMenu.Press(ConsoleHome::PageUp); // LB: show Fable again
    hideMenu.Draw(canvas,"19:46");
    ConsoleHome showAgain(dir,roots); showAgain.Load();
    showAgain.Press(ConsoleHome::Accept);
    showAgain.Press(ConsoleHome::Accept);
    assert(showAgain.Press(ConsoleHome::Accept)==
           (original/"default.xex").string());

    // A GOD/STFS package is catalogued but never passed to XLaunchNewImage.
    const fs::path godDir=root/"Content"/"0000000000000000"/"4D5307E6"/"00007000";
    fs::create_directories(godDir);
    const fs::path godFile=godDir/"default.xex"; // Malicious-looking package must remain non-launchable
    std::vector<unsigned char> stfs(0x1711u,0u);
    stfs[0]='L';stfs[1]='I';stfs[2]='V';stfs[3]='E';
    stfs[0x344u+2]=0x70;                 // GOD 0x00007000
    stfs[0x360u]=0x4d;stfs[0x361u]=0x53;
    stfs[0x362u]=0x07;stfs[0x363u]=0xe6;
    const char* godName="AAAA GOD";
    for(int i=0;godName[i];++i)
        stfs[0x411u+2u*(size_t)i+1u]=(unsigned char)godName[i];
    {std::ofstream pkg(godFile,std::ios::binary);
     pkg.write(reinterpret_cast<const char*>(&stfs[0]),stfs.size());}
    ConsoleHome godMenu(dir,roots); godMenu.Load();
    godMenu.Press(ConsoleHome::Accept);   // Library
    for(int i=0;i<4;++i) godMenu.Press(ConsoleHome::Right); // Packages
    godMenu.Press(ConsoleHome::Accept);   // Details
    assert(godMenu.Press(ConsoleHome::Accept).empty()); // Safe: no XEX launch
    godMenu.Draw(canvas,"19:48");

    // Series360 collection workflow: create, assign, rename, remove and delete
    // entirely with the controller; no title data is modified.
    ConsoleHome collectionMenu(dir,roots); collectionMenu.Load();
    collectionMenu.Press(ConsoleHome::Accept);  // Library
    for(int i=0;i<5;++i) collectionMenu.Press(ConsoleHome::Right); // Collections
    collectionMenu.Press(ConsoleHome::PageDown);  // Manage
    collectionMenu.Press(ConsoleHome::Accept);    // + New collection
    collectionMenu.Press(ConsoleHome::Accept);    // Type "A"
    collectionMenu.Press(ConsoleHome::Refresh);   // Save collection name
    CollectionStore collectionData((root/"aurorax-collections.txt").string());
    assert(collectionData.Load());
    assert(collectionData.Items().size()==1);
    assert(collectionData.Items()[0].name=="A");
    for(int i=0;i<2;++i) collectionMenu.Press(ConsoleHome::Left); // Original
    collectionMenu.Press(ConsoleHome::PageDown);  // Add selected Fable
    collectionMenu.Press(ConsoleHome::Accept);    // Toggle membership
    assert(collectionData.Load());
    assert(collectionData.Contains(0,(original/"default.xex").string()));
    collectionMenu.Press(ConsoleHome::Accept);    // Collection game details
    assert(collectionMenu.Press(ConsoleHome::Accept)==
           (original/"default.xex").string());
    collectionMenu.Press(ConsoleHome::Back);
    collectionMenu.Press(ConsoleHome::PageDown);  // Manage selected collection
    collectionMenu.Press(ConsoleHome::Favorite);  // X: Rename
    collectionMenu.Press(ConsoleHome::Right);     // B key on keyboard
    collectionMenu.Press(ConsoleHome::Accept);    // Append B => AB
    collectionMenu.Press(ConsoleHome::Refresh);   // Save
    assert(collectionData.Load());
    assert(collectionData.Items()[0].name=="AB");
    collectionMenu.Press(ConsoleHome::PageDown);  // Manage membership
    collectionMenu.Press(ConsoleHome::Accept);    // Remove Fable
    assert(collectionData.Load());
    assert(collectionData.Items()[0].games.empty());
    collectionMenu.Press(ConsoleHome::PageDown);  // Manage empty collection
    collectionMenu.Press(ConsoleHome::Refresh);   // Y: request delete
    collectionMenu.Press(ConsoleHome::Refresh);   // Y: confirm delete
    assert(collectionData.Load() && collectionData.Items().empty());
    collectionMenu.Draw(canvas,"19:49");

    // "Nunca" includes only XEX not yet requested for launch from AuroraX.
    const fs::path fresh=root/"Games"/"NeverOpened"/"default.xex";
    fs::create_directories(fresh.parent_path());
    std::ofstream(fresh)<<"unlaunched game fixture";
    ConsoleHome neverMenu(dir,roots); neverMenu.Load();
    neverMenu.Press(ConsoleHome::Accept); // Library
    for(int i=0;i<6;++i) neverMenu.Press(ConsoleHome::Right); // Nunca
    neverMenu.Press(ConsoleHome::Accept); // Details
    assert(neverMenu.Press(ConsoleHome::Accept)==fresh.string());
    neverMenu.Press(ConsoleHome::Back);
    neverMenu.Press(ConsoleHome::Refresh); // Rebuild filters with play ledger
    assert(neverMenu.Press(ConsoleHome::Accept).empty());
    {
        std::ifstream ledger(root/"aurorax-profile.txt");
        const std::string data((std::istreambuf_iterator<char>(ledger)),
                               std::istreambuf_iterator<char>());
        assert(data.find(std::string("P\t")+fresh.string())!=std::string::npos);
    }

    // Per-title content view: controller can browse all three categories,
    // without launching a TU, deleting saves, or changing installed files.
    ConsoleHome contentMenu(dir,roots); contentMenu.Load();
    contentMenu.Press(ConsoleHome::Accept);
    for(int i=0;i<4;++i) contentMenu.Press(ConsoleHome::Right); // Pacotes
    contentMenu.Press(ConsoleHome::Accept); // GOD details
    contentMenu.Press(ConsoleHome::PageDown); // RB: content manager
    contentMenu.Draw(canvas,"19:52");
    contentMenu.Press(ConsoleHome::Right); // DLC
    assert(contentMenu.Press(ConsoleHome::Accept).empty());
    contentMenu.Press(ConsoleHome::Right); // Saves
    assert(contentMenu.Press(ConsoleHome::Accept).empty());
    contentMenu.Press(ConsoleHome::Refresh);
    contentMenu.Press(ConsoleHome::Back); // Original game details
    assert(contentMenu.Press(ConsoleHome::Accept).empty()); // GOD launch blocked

    // Files: isolated workspace navigation, clipboard and explicit confirmation.
    const fs::path fileRoot=root/"FileMenuFixture";
    fs::create_directories(fileRoot);
    std::ofstream(fileRoot/"source.txt")<<"local file";
    ConsoleHome fileMenu(fileRoot.string()+"/",std::vector<std::string>());
    fileMenu.Load();
    fileMenu.Press(ConsoleHome::Right);  // IPTV tile
    fileMenu.Press(ConsoleHome::Down);   // Arquivos
    fileMenu.Press(ConsoleHome::Accept);
    fileMenu.Press(ConsoleHome::Favorite); // X marks source.txt
    fileMenu.Press(ConsoleHome::Guide);    // Start: create personal workspace
    fileMenu.Press(ConsoleHome::Refresh);  // Y confirms prefilled UserFiles
    assert(fs::is_directory(fileRoot/"UserFiles"));
    fileMenu.Press(ConsoleHome::Accept);   // first row is UserFiles
    fileMenu.Press(ConsoleHome::PageUp);   // LB requests paste
    fileMenu.Press(ConsoleHome::Back);     // cancel
    assert(!fs::exists(fileRoot/"UserFiles"/"source.txt"));
    fileMenu.Press(ConsoleHome::PageUp);
    fileMenu.Press(ConsoleHome::Accept);   // confirm paste
    assert(fs::exists(fileRoot/"UserFiles"/"source.txt"));
    assert(fs::exists(fileRoot/"source.txt"));
    fileMenu.Press(ConsoleHome::Search);   // RS opens rename keyboard
    for(int i=0;i<10;++i)fileMenu.Press(ConsoleHome::Favorite);
    fileMenu.Press(ConsoleHome::Accept);   // type A
    fileMenu.Press(ConsoleHome::Refresh);  // confirm rename
    assert(fs::exists(fileRoot/"UserFiles"/"A"));
    assert(!fs::exists(fileRoot/"UserFiles"/"source.txt"));
    fileMenu.Draw(canvas,"19:55");

    fs::remove_all(root);
}
