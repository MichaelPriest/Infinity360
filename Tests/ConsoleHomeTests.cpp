#include "WeatherJson.h"
#include "ConsoleHome.h"
#include "LaunchStatistics.h"
#include "DashboardLayout.h"
#include "ApprovedHome.h"
#include "InfinityIcons.h"
#include "GameGrid.h"
#include "DashboardScreenshot.h"
#include "MediaUrl.h"
#include <cassert>
#include <algorithm>
#include <chrono>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>

// Explicitly exercise historic controller routes in legacy regression fixtures.
class LegacyFixtureHome : public ConsoleHome {
public:
    LegacyFixtureHome(const std::string& dir,const std::vector<std::string>& roots)
        : ConsoleHome(dir,roots,DashboardLayout::Classic){}
};

static int captureScreens(const char* atlas,const char* destination) {
    namespace fs=std::filesystem;
    fs::path output(destination);
    fs::create_directories(output);
    const fs::path root=fs::temp_directory_path()/("infinity-capture-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root/"Games");
    fs::create_directories(root/"UserFiles");
    const std::string dir=root.string()+"/";
    const char* games[]={"Forza Horizon","Halo 3","Gears of War 3","Minecraft","Fable III","Castle Crashers"};
    for(const char* name:games) {
        fs::create_directories(root/"Games"/name);
        // A synthetic XEX2 header supplies metadata without a commercial executable.
        unsigned char raw[160]={};
        auto put=[&](size_t at,unsigned v){for(unsigned i=0;i<4;++i)raw[at+i]=static_cast<unsigned char>(v>>(24-8*i));};
        put(0,0x58455832);put(8,128);put(0x14,1);put(24,0x00040006);put(28,64);
        put(64,0x12345678);put(68,0x20010002);put(72,0x20000000);
        unsigned title=0x494e4600u;
        for(const char* p=name;*p;++p)title=title*31u+static_cast<unsigned char>(*p);
        put(76,title);raw[82]=1;raw[83]=1;
        std::ofstream out(root/"Games"/name/"default.xex",std::ios::binary);
        out.write(reinterpret_cast<const char*>(raw),sizeof raw);
        const auto hex=[](unsigned v){char b[9];std::snprintf(b,sizeof b,"%08X",v);return std::string(b);};
        // Optional authorized artwork inputs for a real-render capture; default CI
        // remains an honest no-art capture. Use the game folder names above.
        if(const char* artRoot=std::getenv("INFINITY_CAPTURE_ART_DIR")){
            const auto source=fs::path(artRoot)/name;
            const auto target=root/"Art"/hex(title);
            for(const auto& art: {std::make_pair("cover.rgba",160u*240u*4u),std::make_pair("background.rgba",640u*360u*4u)}){
                std::error_code ec;
                if(fs::is_regular_file(source/art.first,ec)&&!ec&&fs::file_size(source/art.first,ec)==art.second&&!ec){
                    fs::create_directories(target);
                    fs::copy_file(source/art.first,target/art.first,fs::copy_options::overwrite_existing);
                }
            }
        }
        const auto tu=root/"Content/0000000000000000"/hex(title)/"000B0000";
        fs::create_directories(tu);
        std::string package(0x1800,'\0');package.replace(0,4,"LIVE");
        const auto write=[&](size_t at,unsigned v){for(unsigned i=0;i<4;++i)package[at+i]=static_cast<char>(v>>(24-8*i));};
        write(0x344,0xb0000);write(0x354,0x12345678);write(0x358,0x20010003);write(0x35c,0x20000000);write(0x360,title);
        std::ofstream pkg(tu/"tu00000003_00000000",std::ios::binary);pkg.write(package.data(),package.size());
    }
    {std::ofstream out(root/"aurorax-profile.txt");
     out<<"R\t"<<(root/"Games"/"Halo 3"/"default.xex").string()<<"\n";
     out<<"R\t"<<(root/"Games"/"Forza Horizon"/"default.xex").string()<<"\n";
     out<<"R\t"<<(root/"Games"/"Gears of War 3"/"default.xex").string()<<"\n";
     out<<"F\t"<<(root/"Games"/"Forza Horizon"/"default.xex").string()<<"\n";}
    {std::ofstream out(root/"aurorax-store.tsv");
     for(int i=0;i<5;++i)out<<"demo"<<i<<"\thomebrew\t"<<(i==0?"Demo Homebrew":i==1?"Retro Player":i==2?"Media Tools":i==3?"Demo Theme":"File Utility")
        <<"\t1.0\t"<<(i+1)*1048576<<"\t"<<std::string(64,'a')<<"\thttps://example.invalid/demo"<<i<<".zip\tDemonstration only\n";}
    {std::ofstream out(root/"infinity360-native-queue.tsv");
     for(int i:{2,0,1})out<<"demo"<<i<<"\t"<<std::string(64,'a')<<"\n";}
    {std::ofstream out(root/"playlist.m3u");
     out<<"#EXTM3U\n";
     for(int i=1;i<=5;++i)out<<"#EXTINF:-1 group-title=\"TV ao vivo\",Canal demonstrativo "<<i<<"\nhttps://example.invalid/stream"<<i<<".ts\n";}
    {const auto p=root/"Plugins"/"Demo";fs::create_directories(p);std::ofstream(p/"plugin.axplugin")<<"schema=1\nid=demo.local\nname=Demo local plugin\nversion=1.0\ncategory=app\nentry=default.xex\nenabled=true\n";std::ofstream x(p/"default.xex",std::ios::binary);char header[24]={'X','E','X','2'};x.write(header,sizeof header);}
    {std::ofstream out(root/"launch.ini");
     out<<"[Plugins]\nplugin1=Hdd1:\\LiveEmulation.xex\nplugin2=\nplugin3=\nplugin4=\nplugin5=\n";}
    {std::ofstream out(root/"infinity360-events.log");out<<"N1 U 2026-11-25 21:41Z | Download nativo verificado\nN1 U 2026-11-25 21:40Z | Backup recusado\n2026-11-25 21:30Z | FTP iniciado\n";}
    HomeCanvas canvas;
    if(!canvas.LoadArt(atlas)){fs::remove_all(root);return 2;}
    const std::vector<std::string> roots(1,(root/"Games").string());
    const auto save=[&](const char* name,ConsoleHome& menu) {
        menu.Draw(canvas,"21:42",1795570920);
        std::ofstream out(output/(std::string(name)+".ppm"),std::ios::binary);
        out<<"P6\n1280 720\n255\n";
        const unsigned* pixel=canvas.Pixels();
        for(size_t i=0;i<1280*720;++i) {
            const char rgb[]={static_cast<char>(pixel[i]>>16),static_cast<char>(pixel[i]>>8),static_cast<char>(pixel[i])};
            out.write(rgb,3);
        }
        return !!out;
    };
    bool ok=true;
    ConsoleHome approved(dir,roots);approved.Load();assert(approved.ApprovedHeader());
    ok=save("01-inicio",approved)&&ok;
    approved.Press(ConsoleHome::Search);assert(approved.QuickGuideOpen());ok=save("40-guia-rapido",approved)&&ok;
    approved.Press(ConsoleHome::Favorite);approved.Press(ConsoleHome::PageDown);ok=save("41-guia-edicao",approved)&&ok;
    approved.Press(ConsoleHome::Back);assert(approved.QuickGuideOrder()==QuickGuide::Default());approved.Press(ConsoleHome::Back);assert(!approved.QuickGuideOpen());
    LegacyFixtureHome menu(dir,roots);menu.Load();
    menu.Press(ConsoleHome::Accept);ok=save("02-biblioteca",menu)&&ok;
    menu.Press(ConsoleHome::Accept);ok=save("03-detalhes",menu)&&ok;
    menu.Press(ConsoleHome::PageDown);ok=save("10-conteudo-tu",menu)&&ok;
    for(const char* name:{"11-dlc","12-saves","13-trainers","14-tu-baixadas","15-backups","16-tu-arquivadas","26-tu-retiradas"}){
        menu.Press(ConsoleHome::Right);ok=save(name,menu)&&ok;
    }
    LegacyFixtureHome apps(dir,roots);apps.Load();apps.Press(ConsoleHome::Down);apps.Press(ConsoleHome::Accept);
    ok=save("17-aplicativos",apps)&&ok;
    LegacyFixtureHome emu(dir,roots);emu.Load();emu.Press(ConsoleHome::Down);emu.Press(ConsoleHome::Right);emu.Press(ConsoleHome::Accept);
    ok=save("18-emuladores",emu)&&ok;
    LegacyFixtureHome files(dir,roots);files.Load();files.Press(ConsoleHome::Right);files.Press(ConsoleHome::Down);files.Press(ConsoleHome::Accept);
    ok=save("19-arquivos",files)&&ok;
    LegacyFixtureHome store(dir,roots);store.Load();store.Press(ConsoleHome::PageDown);
    ok=save("04-loja",store)&&ok;
    store.Press(ConsoleHome::Accept);ok=save("05-produto",store)&&ok;
    LegacyFixtureHome tv(dir,roots);tv.Load();tv.Press(ConsoleHome::Right);tv.Press(ConsoleHome::Accept);
    tv.Press(ConsoleHome::Right);ok=save("06-iptv",tv)&&ok;
    LegacyFixtureHome system(dir,roots);system.Load();
    system.Press(ConsoleHome::Right);system.Press(ConsoleHome::Down);
    system.Press(ConsoleHome::Right);system.Press(ConsoleHome::Accept);
    ok=save("07-sistema",system)&&ok;
    const char* pages[]={"20-armazenamento","08-rede","21-sensores","22-downloads","09-plugins","23-eventos","24-dvd","25-configuracoes"};
    for(int i=0;i<8;++i){
        system.Press(ConsoleHome::Accept);ok=save(pages[i],system)&&ok;
        if(i==1){system.Press(ConsoleHome::PageDown);ok=save("29-rede-udp",system)&&ok;system.Press(ConsoleHome::Search);ok=save("50-link-opcoes",system)&&ok;system.Press(ConsoleHome::Back);system.Press(ConsoleHome::Guide);ok=save("43-configurar-rede",system)&&ok;system.Press(ConsoleHome::Back);system.Press(ConsoleHome::PageUp);system.Press(ConsoleHome::Guide);ok=save("48-smb",system)&&ok;system.Press(ConsoleHome::Guide);ok=save("49-smb-config",system)&&ok;system.Press(ConsoleHome::Back);system.Press(ConsoleHome::Back);}
        if(i==2){system.Press(ConsoleHome::PageDown);ok=save("28-video-kernel",system)&&ok;system.Press(ConsoleHome::PageUp);system.Press(ConsoleHome::Guide);ok=save("45-temperaturas",system)&&ok;system.Press(ConsoleHome::PageDown);ok=save("47-refrigeracao",system)&&ok;system.Press(ConsoleHome::PageUp);system.Press(ConsoleHome::Guide);}
        if(i==4){system.Press(ConsoleHome::Guide);ok=save("44-plugins-locais",system)&&ok;system.Press(ConsoleHome::Guide);}
        if(i==5){system.Press(ConsoleHome::PageDown);ok=save("38-notificacoes-pendentes",system)&&ok;system.Press(ConsoleHome::PageDown);ok=save("39-notificacoes-alertas",system)&&ok;system.Press(ConsoleHome::Favorite);assert(system.UnreadNotifications()==0);}
        if(i==3){
            system.Press(ConsoleHome::PageDown);ok=save("27-fila-store",system)&&ok;
            system.Press(ConsoleHome::PageDown);
        }
        system.Press(ConsoleHome::Back);
        if(i<7)system.Press(ConsoleHome::Right);
        else {
            system.Press(ConsoleHome::Accept);
            for(const char* name:{"30-tema-azul","31-tema-ambar","32-tema-violeta","33-tema-vermelho","34-tema-ciano","35-tema-grafite"}){
                system.Press(ConsoleHome::Right);ok=save(name,system)&&ok;
            }
            system.Press(ConsoleHome::Right);
            system.Press(ConsoleHome::PageDown);assert(system.Language()==1);ok=save("36-english",system)&&ok;
            system.Press(ConsoleHome::PageDown);assert(system.Language()==2);ok=save("37-espanol",system)&&ok;
            {LegacyFixtureHome persisted(dir,roots);persisted.Load();assert(persisted.Language()==2);}
            system.Press(ConsoleHome::PageDown);assert(system.Language()==0);
            system.Press(ConsoleHome::Down);ok=save("42-marcos-locais",system)&&ok;system.Press(ConsoleHome::Accept);ok=save("52-conquistas-gpd",system)&&ok;system.Press(ConsoleHome::Back);system.Press(ConsoleHome::Favorite);ok=save("53-aparencia",system)&&ok;system.Press(ConsoleHome::Back);system.Press(ConsoleHome::Guide);ok=save("51-perfis-saves",system)&&ok;system.Press(ConsoleHome::Back);system.Press(ConsoleHome::Up);system.Press(ConsoleHome::Accept);ok=save("46-energia",system)&&ok;system.Press(ConsoleHome::Back);system.Press(ConsoleHome::Back);
        }
    }
    // Real renderer captures for the nine modern home layouts and cover grid.
    for(unsigned style=1;style<DashboardLayout::Count;++style){
        {std::ofstream out(root/"aurorax-profile.txt",std::ios::app);out<<"J\t"<<style<<"\nG\t2\n";}
        LegacyFixtureHome modern(dir,roots);modern.Load();
        const auto name=std::to_string(53+style)+"-layout-"+std::to_string(style);
        ok=save(name.c_str(),modern)&&ok;
        if(style==DashboardLayout::TopIcons){modern.Press(ConsoleHome::Right);modern.Press(ConsoleHome::Accept);ok=save("63-biblioteca-grade",modern)&&ok;
            modern.Press(ConsoleHome::Up);ok=save("64-biblioteca-filtros",modern)&&ok;
            modern.Press(ConsoleHome::Down);modern.Press(ConsoleHome::Accept);ok=save("65-detalhes-moderno",modern)&&ok;
            modern.Press(ConsoleHome::Back);modern.Press(ConsoleHome::Back);modern.Press(ConsoleHome::PageDown);ok=save("66-store-moderna",modern)&&ok;
            modern.Press(ConsoleHome::Accept);ok=save("67-store-produto-moderno",modern)&&ok;
            modern.Press(ConsoleHome::Back);modern.Press(ConsoleHome::Search);ok=save("68-fontes-store",modern)&&ok;
            modern.Press(ConsoleHome::Back);modern.Press(ConsoleHome::Back);
            modern.Press(ConsoleHome::Right);modern.Press(ConsoleHome::Right);modern.Press(ConsoleHome::Accept);ok=save("69-iptv-moderno",modern)&&ok;
            LegacyFixtureHome profile(dir,roots);profile.Load();
            InfinitySocial::Snapshot fixture;
            fixture.slots[0]={InfinitySocial::SignIn::Local,InfinitySocial::Origin::Test,false,"PerfilTeste","",false,0};
            profile.SetSocialSnapshot(fixture,0);
            assert(profile.SocialHeader().origin==InfinitySocial::Origin::Test);
            ok=save("70-perfil-teste",profile)&&ok;
            profile.Press(ConsoleHome::Search);
            assert(profile.QuickGuideOpen());
            profile.Press(ConsoleHome::PageUp);
            assert(!profile.TakeSigninRequest()); // Host never opens a native Xbox dialog.
            assert(profile.QuickGuideOpen());
            ok=save("71-guia-perfil-teste",profile)&&ok;
        }
    }
    fs::remove_all(root);
    return ok?0:3;
}

int main(int argc,char** argv) {
    if(argc==3)return captureScreens(argv[1],argv[2]);
    assert(argc==2);
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/("aurorax-home-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    {const auto p=root/"ApprovedDefault";fs::create_directories(p);
     const std::vector<std::string> noGameRoots;
     ConsoleHome fresh(p.string()+"/",noGameRoots);fresh.Load();assert(fresh.ApprovedHeader());
     std::ofstream(p/"aurorax-profile.txt")<<"J\t0\n";
     ConsoleHome migrated(p.string()+"/",noGameRoots);migrated.Load();assert(migrated.ApprovedHeader());
     std::ofstream(p/"aurorax-profile.txt")<<"J\t0\nM\t1\n";
     ConsoleHome explicitClassic(p.string()+"/",noGameRoots);explicitClassic.Load();assert(!explicitClassic.ApprovedHeader());
    }
    {using namespace ApprovedHome;
     assert(Pages(0)==1&&Pages(6)==1&&Pages(7)==2&&Pages(13)==3);
     assert(Page(99,7)==1&&Visible(0,7)==6&&Visible(1,7)==1&&Visible(2,0)==0);
     assert(Featured(4,8)==0&&Featured(3,2)==1&&Featured(99,0)==0);
     assert(ValidOrder("012345")&&ValidOrder("543210")&&!ValidOrder("012344")&&!ValidOrder("01234"));
     assert(Move(LibraryLink,3,6)>=FirstCover&&Move(LibraryLink,3,6)<Weather);
    }
    fs::create_directories(root/"Games"/"Halo 3");
    const std::string exe=(root/"Games"/"Halo 3"/"default.xex").string();
    std::ofstream(exe)<<"fixture";
    const std::string dir=root.string()+"/";
    const std::vector<std::string> roots(1,(root/"Games").string());
    LegacyFixtureHome menu(dir,roots);
    menu.Load();
    HomeCanvas canvas;
    assert(canvas.LoadArt(argv[1]));
    // Each approved vector glyph must produce visible ink in the actual framebuffer.
    {HomeCanvas icons;for(int kind=0;kind<28;++kind){icons.SeriesBackground();unsigned before=icons.Pixels()[105*HomeCanvas::Width+105];(void)before;InfinityIcons::Draw(icons,kind,100,100,32,0xffffffffu);bool visible=false;for(int y=98;y<136;++y)for(int x=98;x<136;++x)if((icons.Pixels()[y*HomeCanvas::Width+x]&0xffffffu)>0x999999u)visible=true;assert(visible);}}
    // Exercise actual framebuffer composition, not a screenshot mockup.
    {
        HomeCanvas modern;modern.SeriesBackground();
        const unsigned original=modern.Pixels()[100*HomeCanvas::Width+100];
        modern.Translucent(80,80,40,40,0xffffffffu,0,6);
        assert(modern.Pixels()[100*HomeCanvas::Width+100]==original);
        modern.Translucent(80,80,40,40,0xffffffffu,128,6);
        const unsigned mixed=modern.Pixels()[100*HomeCanvas::Width+100];
        assert(mixed!=original&&mixed!=0xffffffffu);
        modern.SeriesTile(200,200,80,70,true);
        assert(modern.Pixels()[200*HomeCanvas::Width+240]==0xffa8ff00u);
        assert(modern.Pixels()[201*HomeCanvas::Width+201]!=0xffa8ff00u); // rounded corner, not a square outline
    }

    // A 2:1 source with 2:3 pixel aspect displays as 4:3, not stretched.
    const unsigned char rgb[]={255,0,0,255,0,0};
    canvas.VideoRgb(rgb,2,1,2.0/3.0);
    assert(canvas.Pixels()[360*1280+159]==0xff000000u);
    assert(canvas.Pixels()[360*1280+160]==0xffff0000u);
    assert(canvas.Pixels()[360*1280+1119]==0xffff0000u);
    assert(canvas.Pixels()[360*1280+1120]==0xff000000u);
    canvas.VideoRgb(rgb,2,1,1.0);
    assert(canvas.Pixels()[360*1280+0]==0xffff0000u);
    menu.Draw(canvas,"19:42");
    // The active Library tile now uses the approved rounded lime focus,
    // rather than the legacy opaque green border. Verify an actual mid-edge pixel.
    assert(canvas.Pixels()[200*1280+42]==0xffa8ff00u);
    menu.Press(ConsoleHome::Accept);                 // Library
    menu.Press(ConsoleHome::Favorite);
    assert(menu.Press(ConsoleHome::Accept).empty()); // Details
    {std::ifstream profile(root/"aurorax-profile.txt");
     const std::string data((std::istreambuf_iterator<char>(profile)),
                            std::istreambuf_iterator<char>());
     assert(data.find("R\t")==std::string::npos); // Not recently played yet
    }
    assert(menu.Press(ConsoleHome::Accept)==exe);
    LegacyFixtureHome restored(dir,roots); restored.Load();
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
    LegacyFixtureHome mediaMenu(dir,roots);mediaMenu.Load();mediaMenu.Press(ConsoleHome::Right);
    mediaMenu.Press(ConsoleHome::Accept);mediaMenu.Press(ConsoleHome::Accept);
    assert(mediaMenu.Press(ConsoleHome::Accept).empty());assert(mediaMenu.TakeMovie()==dir+"clip.wmv");
    std::ofstream(root/"playlist.m3u")<<"#EXTM3U\n#EXTINF:-1,Canal na rede\nhttp://192.168.1.10:8090/channel/id.mpg\n"
        "#EXTINF:-1,HLS original\nhttps://example.invalid/live.m3u8\n#EXTINF:-1,Nexus\nbrazucaplay:sample\n";
    LegacyFixtureHome streamMenu(dir,roots);streamMenu.Load();streamMenu.Press(ConsoleHome::Right);
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
    {std::ofstream out(music/"default.xex",std::ios::binary);char header[24]={'X','E','X','2'};out.write(header,sizeof header);}
    std::ofstream(music/"plugin.axplugin")
        <<"schema=1\n"<<"id=aurorax.music\n"<<"name=Music Player\n"
        <<"version=0.1\n"<<"category=app\n"
        <<"entry=default.xex\n"<<"enabled=true\n";
    const fs::path emulator=root/"Plugins"/"Retro";
    fs::create_directories(emulator);
    {std::ofstream out(emulator/"default.xex",std::ios::binary);char header[24]={'X','E','X','2'};out.write(header,sizeof header);}
    std::ofstream(emulator/"plugin.axplugin")
        <<"schema=1\n"<<"id=aurorax.retro\n"<<"name=Retro Plugin\n"
        <<"version=0.1\n"<<"category=emulator\n"
        <<"entry=default.xex\n"<<"enabled=true\n";
    LegacyFixtureHome pluginMenu(dir,roots); pluginMenu.Load();
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
    LegacyFixtureHome tabsMenu(dir,roots); tabsMenu.Load();
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
    LegacyFixtureHome sysMenu(dir,roots); sysMenu.Load();
    sysMenu.Press(ConsoleHome::Right);  // Cloud360
    sysMenu.Press(ConsoleHome::Down);   // Arquivos
    sysMenu.Press(ConsoleHome::Right);  // Sistema
    sysMenu.Press(ConsoleHome::Accept);
    sysMenu.Draw(canvas,"19:45");
    std::vector<unsigned> storageScreen(canvas.Pixels(),
        canvas.Pixels()+HomeCanvas::Width*HomeCanvas::Height);
    // Storage details: actual portable filesystem snapshot, refresh and return.
    sysMenu.Press(ConsoleHome::Accept);
    sysMenu.Draw(canvas,"19:45");
    assert(!std::equal(storageScreen.begin(),storageScreen.end(),canvas.Pixels()));
    sysMenu.Press(ConsoleHome::Refresh);
    sysMenu.Draw(canvas,"19:45");
    sysMenu.Press(ConsoleHome::Back);
    sysMenu.Press(ConsoleHome::Right);  // Rede e FTP card
    sysMenu.Draw(canvas,"19:45");
    assert(!std::equal(storageScreen.begin(),storageScreen.end(),canvas.Pixels()));
    sysMenu.Press(ConsoleHome::Accept);
    sysMenu.Draw(canvas,"19:45");
    sysMenu.Press(ConsoleHome::Refresh); // refresh network without leaving details
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
    LegacyFixtureHome extraMenu(dir,roots); extraMenu.Load();
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
    LegacyFixtureHome categoryMenu(dir,roots); categoryMenu.Load();
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
    LegacyFixtureHome orderedMenu(dir,roots); orderedMenu.Load();
    orderedMenu.Press(ConsoleHome::Accept); // Library: A-Z
    orderedMenu.Press(ConsoleHome::Search); // RS -> Z-A
    orderedMenu.Press(ConsoleHome::Accept);
    assert(orderedMenu.Press(ConsoleHome::Accept)==exe); // Halo 3 sorts first
    orderedMenu.Press(ConsoleHome::Back);
    orderedMenu.Press(ConsoleHome::Search); // RS -> device ordering
    orderedMenu.Draw(canvas,"19:45");

    // Library search: Start opens on-screen keyboard, A types, Y applies.
    LegacyFixtureHome searchMenu(dir,roots); searchMenu.Load();
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
    LegacyFixtureHome hideMenu(dir,roots); hideMenu.Load();
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
    LegacyFixtureHome showAgain(dir,roots); showAgain.Load();
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
    LegacyFixtureHome godMenu(dir,roots); godMenu.Load();
    godMenu.Press(ConsoleHome::Accept);   // Library
    for(int i=0;i<4;++i) godMenu.Press(ConsoleHome::Right); // Packages
    godMenu.Press(ConsoleHome::Accept);   // Details
    assert(godMenu.Press(ConsoleHome::Accept).empty()); // Safe: no XEX launch
    godMenu.Draw(canvas,"19:48");

    // Series360 collection workflow: create, assign, rename, remove and delete
    // entirely with the controller; no title data is modified.
    LegacyFixtureHome collectionMenu(dir,roots); collectionMenu.Load();
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
    LegacyFixtureHome neverMenu(dir,roots); neverMenu.Load();
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
    LegacyFixtureHome contentMenu(dir,roots); contentMenu.Load();
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
    LegacyFixtureHome fileMenu(fileRoot.string()+"/",std::vector<std::string>());
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
    assert(fileMenu.TransferRunning());
    for(int i=0;i<20 && fileMenu.TransferRunning();++i)fileMenu.Tick();
    assert(!fileMenu.TransferRunning());
    assert(fs::exists(fileRoot/"UserFiles"/"source.txt"));
    assert(fs::exists(fileRoot/"source.txt"));
    fileMenu.Press(ConsoleHome::Search);   // RS opens rename keyboard
    for(int i=0;i<10;++i)fileMenu.Press(ConsoleHome::Favorite);
    fileMenu.Press(ConsoleHome::Accept);   // type A
    fileMenu.Press(ConsoleHome::Refresh);  // confirm rename
    assert(fs::exists(fileRoot/"UserFiles"/"A"));
    assert(!fs::exists(fileRoot/"UserFiles"/"source.txt"));
    fileMenu.Draw(canvas,"19:55");

    // Pins are independent of stars, survive restart, and obey hidden filtering.
    const fs::path pinRoot=root/"PinFixture";
    const fs::path pinGame=pinRoot/"Games"/"Alpha"/"default.xex";
    fs::create_directories(pinGame.parent_path());
    std::ofstream(pinGame)<<"pin fixture";
    const std::string pinDir=pinRoot.string()+"/";
    const std::vector<std::string> pinRoots(1,(pinRoot/"Games").string());
    LegacyFixtureHome pinMenu(pinDir,pinRoots); pinMenu.Load();
    pinMenu.Press(ConsoleHome::Accept); // Library
    pinMenu.Press(ConsoleHome::Accept); // Details
    pinMenu.Press(ConsoleHome::PageUp); // LB pins, does not launch
    {
        std::ifstream in(pinRoot/"aurorax-profile.txt");
        const std::string data((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
        assert(data.find("K\t"+pinGame.string())!=std::string::npos);
        assert(data.find("F\t"+pinGame.string())==std::string::npos);
    }
    LegacyFixtureHome pinReload(pinDir,pinRoots); pinReload.Load();
    pinReload.Draw(canvas,"19:56");
    pinReload.Press(ConsoleHome::Accept);
    for(int i=0;i<8;++i)pinReload.Press(ConsoleHome::Right); // Fixados
    pinReload.Press(ConsoleHome::Accept);
    assert(pinReload.Press(ConsoleHome::Accept)==pinGame.string());
    pinReload.Press(ConsoleHome::Back);
    pinReload.Press(ConsoleHome::PageUp); // hide pinned game from listing
    assert(pinReload.Press(ConsoleHome::Accept).empty());
    pinReload.Press(ConsoleHome::Back); // home
    pinReload.Press(ConsoleHome::Down); // apps
    pinReload.Press(ConsoleHome::Down); // recent
    pinReload.Press(ConsoleHome::Right); // pinned / favorites
    pinReload.Press(ConsoleHome::Accept);
    assert(pinReload.Press(ConsoleHome::Accept).empty());
    pinReload.Press(ConsoleHome::Back);
    pinReload.Press(ConsoleHome::Left);
    pinReload.Press(ConsoleHome::Accept); // recent
    assert(pinReload.Press(ConsoleHome::Accept).empty());
    LegacyFixtureHome unpin(pinDir,pinRoots);unpin.Load();
    unpin.Press(ConsoleHome::Accept);
    for(int i=0;i<7;++i)unpin.Press(ConsoleHome::Right); // hidden
    unpin.Press(ConsoleHome::PageUp); // unhide
    unpin.Press(ConsoleHome::Right); // pins
    unpin.Press(ConsoleHome::Accept);
    unpin.Press(ConsoleHome::PageUp); // unpin
    assert(unpin.Press(ConsoleHome::Accept).empty());
    {
        std::ifstream in(pinRoot/"aurorax-profile.txt");
        const std::string data((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
        assert(data.find("K\t"+pinGame.string())==std::string::npos);
    }

    // Home layout: cancel, save, restart, route to moved tile, restore defaults.
    const fs::path layoutRoot=root/"LayoutFixture";
    fs::create_directories(layoutRoot);
    const std::string layoutDir=layoutRoot.string()+"/";
    const std::vector<std::string> noRoots;
    const fs::path profile=layoutRoot/"aurorax-profile.txt";
    LegacyFixtureHome layout(layoutDir,noRoots);layout.Load();
    layout.Press(ConsoleHome::Guide); // Start editor
    layout.Press(ConsoleHome::Right);
    layout.Press(ConsoleHome::Right); // Files slot
    layout.Press(ConsoleHome::PageUp);
    layout.Press(ConsoleHome::PageUp); // Files, Apps, Emulators, System
    layout.Draw(canvas,"20:01");
    layout.Press(ConsoleHome::Back); // cancel draft
    assert(!fs::exists(profile));
    layout.Press(ConsoleHome::Guide);
    layout.Press(ConsoleHome::Right);
    layout.Press(ConsoleHome::Right);
    layout.Press(ConsoleHome::PageUp);
    layout.Press(ConsoleHome::PageUp);
    layout.Press(ConsoleHome::Refresh); // commit
    {
        std::ifstream in(profile);
        const std::string data((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
        assert(data.find("O\t2013")!=std::string::npos);
    }
    LegacyFixtureHome restoredLayout(layoutDir,noRoots);restoredLayout.Load();
    restoredLayout.Press(ConsoleHome::Down); // first middle slot is now Files
    restoredLayout.Press(ConsoleHome::Accept);
    restoredLayout.Press(ConsoleHome::Guide); // Files creates personal workspace
    restoredLayout.Press(ConsoleHome::Refresh);
    assert(fs::is_directory(layoutRoot/"UserFiles"));
    restoredLayout.Press(ConsoleHome::Back);
    restoredLayout.Press(ConsoleHome::Guide);
    restoredLayout.Press(ConsoleHome::Favorite); // default draft
    restoredLayout.Press(ConsoleHome::Refresh);
    {
        std::ifstream in(profile);
        const std::string data((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
        assert(data.find("O\t0123")!=std::string::npos);
    }
    // Malformed permutations must never index an invalid section.
    std::ofstream(profile)<<"O\t2229\n";
    LegacyFixtureHome invalidLayout(layoutDir,noRoots);invalidLayout.Load();
    invalidLayout.Draw(canvas,"20:02");
    invalidLayout.Press(ConsoleHome::Guide);
    invalidLayout.Press(ConsoleHome::Refresh);
    {
        std::ifstream in(profile);
        const std::string data((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
        assert(data.find("O\t0123")!=std::string::npos);
    }

    // TitleID-based local artwork for packages, bounded raw RGBA and refresh.
    const fs::path artRoot=root/"ArtFixture";
    const fs::path artPkg=artRoot/"Content"/"0000000000000000"/"4D5307E6"/"00007000"/"game";
    const fs::path artFolder=artRoot/"Art"/"4D5307E6";
    fs::create_directories(artPkg.parent_path());
    fs::create_directories(artFolder);
    {std::ofstream out(artPkg,std::ios::binary);
     out.write(reinterpret_cast<const char*>(&stfs[0]),stfs.size());}
    const auto writeArt=[](const fs::path& file,size_t pixels,unsigned char red,unsigned char green) {
        std::vector<unsigned char> rgba(pixels*4);
        for(size_t i=0;i<pixels;++i) {rgba[i*4]=red;rgba[i*4+1]=green;rgba[i*4+3]=255;}
        std::ofstream out(file,std::ios::binary);
        out.write(reinterpret_cast<const char*>(rgba.data()),rgba.size());
    };
    writeArt(artFolder/"cover.rgba",160*240,200,10);
    writeArt(artFolder/"background.rgba",640*360,10,200);
    LegacyFixtureHome artwork(artRoot.string()+"/",noRoots);artwork.Load();
    artwork.Press(ConsoleHome::Accept);
    artwork.Draw(canvas,"20:05");
    assert(canvas.Pixels()[250*1280+60]==0xffc80a00u);
    artwork.Press(ConsoleHome::Favorite);
    artwork.Press(ConsoleHome::Accept);
    artwork.Draw(canvas,"20:05");
    assert(canvas.Pixels()[220*1280+850]==0xff0ac800u);
    assert(canvas.Pixels()[250*1280+100]==0xffc80a00u);
    // Truncation is rejected after explicit reload; cached bytes remain stable until then.
    std::ofstream(artFolder/"cover.rgba",std::ios::binary|std::ios::trunc)<<"bad";
    artwork.Draw(canvas,"20:05");
    assert(canvas.Pixels()[250*1280+100]==0xffc80a00u);
    artwork.Press(ConsoleHome::Refresh); // Back to listing, invalidate cache
    artwork.Draw(canvas,"20:05");
    assert(canvas.Pixels()[250*1280+60]!=0xffc80a00u);
    writeArt(artFolder/"cover.rgba",160*240,200,10);
    artwork.Press(ConsoleHome::Refresh);
    artwork.Press(ConsoleHome::Back); // home, favorite card
    artwork.Draw(canvas,"20:05");
    assert(canvas.Pixels()[520*1280+860]==0xffc80a00u);
    // Direct renderer rejects trailing bytes and invalid destinations.
    canvas.ClearLocalArt();
    {std::ofstream out(artFolder/"cover.rgba",std::ios::binary|std::ios::app);out.put('x');}
    assert(!canvas.LocalArt((artFolder/"cover.rgba").string(),true,0,0,30,45));
    assert(!canvas.LocalArt((artFolder/"background.rgba").string(),false,1200,0,100,100));

    // Controller artwork editor: preview, independent variants, cancellation,
    // persistence across restart, and non-destructive reset.
    writeArt(artFolder/"cover.rgba",160*240,200,10);
    writeArt(artFolder/"cover-alt.rgba",160*240,5,180);
    writeArt(artFolder/"background-alt.rgba",640*360,180,5);
    LegacyFixtureHome artEdit(artRoot.string()+"/",noRoots);artEdit.Load();
    artEdit.Press(ConsoleHome::Accept);
    artEdit.Press(ConsoleHome::Accept);
    artEdit.Press(ConsoleHome::Guide);
    artEdit.Press(ConsoleHome::Right); // alternate cover
    artEdit.Draw(canvas,"20:06");
    assert(canvas.Pixels()[250*1280+155]==0xff05b400u);
    artEdit.Press(ConsoleHome::Back); // cancel
    artEdit.Draw(canvas,"20:06");
    assert(canvas.Pixels()[250*1280+100]==0xffc80a00u);
    artEdit.Press(ConsoleHome::Guide);
    artEdit.Press(ConsoleHome::Right); // alternate cover
    artEdit.Press(ConsoleHome::Down);
    artEdit.Press(ConsoleHome::Left); // original background
    artEdit.Press(ConsoleHome::Refresh); // save
    artEdit.Draw(canvas,"20:06");
    assert(canvas.Pixels()[250*1280+100]==0xff05b400u);
    assert(canvas.Pixels()[220*1280+850]!=0xff0ac800u);
    LegacyFixtureHome artRestored(artRoot.string()+"/",noRoots);artRestored.Load();
    artRestored.Press(ConsoleHome::Accept);
    artRestored.Draw(canvas,"20:07");
    assert(canvas.Pixels()[250*1280+60]==0xff05b400u);
    artRestored.Press(ConsoleHome::Accept);
    artRestored.Press(ConsoleHome::Guide);
    artRestored.Press(ConsoleHome::Favorite); // reset both to main
    artRestored.Press(ConsoleHome::Refresh);
    artRestored.Draw(canvas,"20:07");
    assert(canvas.Pixels()[250*1280+100]==0xffc80a00u);
    assert(canvas.Pixels()[220*1280+850]==0xff0ac800u);
    assert(fs::file_size(artFolder/"cover-alt.rgba")==160u*240u*4u);
    assert(fs::file_size(artFolder/"background-alt.rgba")==640u*360u*4u);

    // Imported titles can rename only an existing, locally verified title.
    {std::ofstream names(artRoot/"aurorax-aurora-names.tsv");
     names<<"4D5307E6\tImported Halo\n";
     names<<"DEADBEEF\tNot installed\n";
     names<<"00000000\tInvalid ID\n";}
    LegacyFixtureHome imported(artRoot.string()+"/",noRoots);imported.Load();
    imported.Press(ConsoleHome::Accept);
    imported.Press(ConsoleHome::Guide); // search by imported name
    // I: second row, second column
    imported.Press(ConsoleHome::Down);imported.Press(ConsoleHome::Right);
    imported.Press(ConsoleHome::Accept);imported.Press(ConsoleHome::Refresh);
    imported.Press(ConsoleHome::Accept);
    imported.Press(ConsoleHome::Guide); // art editor works only from actual details
    imported.Draw(canvas,"20:08");
    assert(canvas.Pixels()[250*1280+155]==0xffc80a00u);
    imported.Press(ConsoleHome::Back);
    assert(imported.Press(ConsoleHome::Accept).empty()); // GOD remains blocked
    // Explicit profile merge: cancellation, verified titles, no fabricated path.
    const fs::path migrateRoot=root/"MigrateProfileFixture";
    const fs::path migratePkg=migrateRoot/"Content"/"0000000000000000"/"4D5307E6"/"00007000"/"game";
    fs::create_directories(migratePkg.parent_path());
    {std::ofstream out(migratePkg,std::ios::binary);
     out.write(reinterpret_cast<const char*>(&stfs[0]),stfs.size());}
    {std::ofstream out(migrateRoot/"aurorax-aurora-profile.tsv");
     out<<"F\t4D5307E6\nF\tDEADBEEF\nR\t4D5307E6\n";}
    const fs::path migrateProfile=migrateRoot/"aurorax-profile.txt";
    LegacyFixtureHome migrate(migrateRoot.string()+"/",noRoots);migrate.Load();
    migrate.Press(ConsoleHome::Favorite); // preview
    migrate.Draw(canvas,"20:09");
    migrate.Press(ConsoleHome::Back);
    assert(!fs::exists(migrateProfile));
    migrate.Press(ConsoleHome::Favorite);
    migrate.Press(ConsoleHome::Accept);
    {
        std::ifstream in(migrateProfile);
        const std::string data((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
        assert(data.find("F\t"+migratePkg.string())!=std::string::npos);
        assert(data.find("DEADBEEF")==std::string::npos);
        assert(data.find("R\t")==std::string::npos); // nonlaunchable package
    }
    LegacyFixtureHome migrateAgain(migrateRoot.string()+"/",noRoots);migrateAgain.Load();
    migrateAgain.Press(ConsoleHome::Favorite);
    migrateAgain.Press(ConsoleHome::Accept);
    {
        std::ifstream in(migrateProfile);
        const std::string data((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());
        const std::string entry="F\t"+migratePkg.string();
        assert(data.find(entry)!=std::string::npos);
        assert(data.find(entry,data.find(entry)+1)==std::string::npos);
    }

    // Trainer catalogue: exact title/media/version, no auto-launch, two confirmations.
    const fs::path trainerRoot=root/"TrainerFixture";
    const fs::path trainerGame=trainerRoot/"Games"/"TestGame"/"default.xex";
    const fs::path trainerFolder=trainerRoot/"Trainers"/"4D5307E6"/"Example";
    fs::create_directories(trainerGame.parent_path());
    fs::create_directories(trainerFolder);
    std::vector<unsigned char> trainerGameHeader(128,0);
    const auto trainerPut=[&](size_t at,unsigned n) {
        for(int i=0;i<4;++i)trainerGameHeader[at+i]=static_cast<unsigned char>(n>>(24-8*i));
    };
    trainerPut(0,0x58455832);trainerPut(8,128);trainerPut(0x14,1);
    trainerPut(24,0x00040006);trainerPut(28,64);
    trainerPut(64,0x12345678);trainerPut(68,0x20010002);trainerPut(76,0x4d5307e6);
    {std::ofstream out(trainerGame,std::ios::binary);
     out.write(reinterpret_cast<const char*>(trainerGameHeader.data()),trainerGameHeader.size());}
#ifdef INFINITY_DISC_INSTALL
    {
        // Drive aliases are ordinary directories inside a private host fixture.
        const auto previous=fs::current_path();
        const auto drives=root/"DvdMenuFixture";
        fs::create_directories(drives/"dvd:");fs::create_directory(drives/"hdd:");fs::create_directory(drives/"usb0:");
        fs::copy_file(trainerGame,drives/"dvd:"/"default.xex");
        fs::current_path(drives);
        LegacyFixtureHome dvdMenu(dir,roots);dvdMenu.Load();
        for(auto key:{ConsoleHome::Right,ConsoleHome::Down,ConsoleHome::Right,ConsoleHome::Accept})dvdMenu.Press(key);
        for(int i=0;i<6;++i)dvdMenu.Press(ConsoleHome::Right);
        dvdMenu.Press(ConsoleHome::Accept); // DVD panel, initially Extract
        dvdMenu.Press(ConsoleHome::Right);dvdMenu.Press(ConsoleHome::Accept); // Confirm GOD on HDD
        DiscInstallRequest request;
        assert(!dvdMenu.TakeDiscInstallRequest(request));
        dvdMenu.Press(ConsoleHome::Down); // Changing destination invalidates the first confirmation
        dvdMenu.Press(ConsoleHome::Accept);assert(!dvdMenu.TakeDiscInstallRequest(request));
        dvdMenu.Press(ConsoleHome::Back);dvdMenu.Press(ConsoleHome::Accept);
        assert(!dvdMenu.TakeDiscInstallRequest(request));
        dvdMenu.Press(ConsoleHome::Accept);
        assert(dvdMenu.TakeDiscInstallRequest(request) && request.format==DiscInstallRequest::Format::God);
        assert(request.mount=="usb0:/" && request.identity.titleId==0x4d5307e6);
        assert(!dvdMenu.TakeDiscInstallRequest(request));
        dvdMenu.Press(ConsoleHome::Left);dvdMenu.Press(ConsoleHome::Accept);dvdMenu.Press(ConsoleHome::Accept);
        assert(dvdMenu.TakeDiscInstallRequest(request) && request.format==DiscInstallRequest::Format::Extracted);
        dvdMenu.Press(ConsoleHome::Left);dvdMenu.Press(ConsoleHome::Accept);dvdMenu.Press(ConsoleHome::Accept);
        assert(dvdMenu.TakeDiscSystemExit());
        fs::current_path(previous);
    }
#endif

    const fs::path trainerLauncher=trainerFolder/"trainer.xex";
    std::ofstream(trainerLauncher,std::ios::binary)<<"XEX2 fixture";
    const fs::path trainerManifest=trainerFolder/"trainer.axtrainer";
    const auto writeTrainer=[&](const char* version,const char* entry) {
        std::ofstream out(trainerManifest);
        out<<"schema=1\nmode=standalone\nname=Example Trainer\ntitle_id=4D5307E6\n"
           <<"media_id=12345678\nversion="<<version<<"\nentry="<<entry<<"\n";
    };
    writeTrainer("20010002","trainer.xex");
    const auto trainerItems=TrainerCatalog::Scan(trainerRoot.string()+"/",0x4d5307e6);
    assert(trainerItems.size()==1);
    XboxExecutionInfo trainerInfo;
    assert(ContentCatalog::ReadExecutionInfo(trainerGame.string(),trainerInfo));
    assert(trainerItems[0].Compatibility(trainerInfo)==TrainerEntry::HeaderMatches);
    trainerInfo.mediaId=1;
    assert(trainerItems[0].Compatibility(trainerInfo)==TrainerEntry::DifferentMedia);
    LegacyFixtureHome trainerMenu(trainerRoot.string()+"/",std::vector<std::string>(1,(trainerRoot/"Games").string()));
    trainerMenu.Load();
    trainerMenu.Press(ConsoleHome::Accept);trainerMenu.Press(ConsoleHome::Accept);
    trainerMenu.Press(ConsoleHome::PageDown);
    for(int i=0;i<3;++i)trainerMenu.Press(ConsoleHome::Right);
    trainerMenu.Draw(canvas,"20:10");
    assert(trainerMenu.Press(ConsoleHome::Accept).empty()); // prompt only
    assert(trainerMenu.Press(ConsoleHome::Back).empty()); // cancel
    assert(trainerMenu.Press(ConsoleHome::Accept).empty());
    assert(trainerMenu.Press(ConsoleHome::Accept)==trainerLauncher.string());
    writeTrainer("20010003","trainer.xex");
    trainerMenu.Press(ConsoleHome::Refresh);
    assert(trainerMenu.Press(ConsoleHome::Accept).empty());
    assert(trainerMenu.Press(ConsoleHome::Accept).empty()); // incompatible
    writeTrainer("20010002","../trainer.xex");
    assert(TrainerCatalog::Scan(trainerRoot.string()+"/",0x4d5307e6).empty());
    writeTrainer("20010002","trainer.xex");
    trainerMenu.Press(ConsoleHome::Refresh);
    assert(trainerMenu.Press(ConsoleHome::Accept).empty());
    fs::remove(trainerLauncher);
    assert(trainerMenu.Press(ConsoleHome::Accept).empty()); // recheck blocks removed file

    std::ofstream(trainerLauncher,std::ios::binary)<<"XEX2 fixture";
    {std::ofstream out(trainerManifest);
     out<<"schema=2\nmode=standalone\nname=TU Trainer\ntitle_id=4D5307E6\n"
        <<"media_id=12345678\nversion=20010002\nentry=trainer.xex\ntu=20020003\n";}
    const auto tuTrainers=TrainerCatalog::Scan(trainerRoot.string()+"/",0x4d5307e6);
    assert(tuTrainers.size()==1);
    assert(tuTrainers[0].CheckUpdates(std::vector<XboxContentPackage>())==TrainerEntry::RequiredMissing);
    XboxContentPackage candidate={};
    candidate.titleId=0x4d5307e6;candidate.mediaId=0x12345678;
    candidate.version=0x20020003;candidate.contentType=0x000b0000;
    std::vector<XboxContentPackage> candidates(1,candidate);
    assert(tuTrainers[0].CheckUpdates(candidates)==TrainerEntry::RequiredLocal);
    candidates.push_back(candidate);
    assert(tuTrainers[0].CheckUpdates(candidates)==TrainerEntry::RequiredAmbiguous);
    trainerMenu.Press(ConsoleHome::Refresh);
    assert(trainerMenu.Press(ConsoleHome::Accept).empty());
    assert(trainerMenu.Press(ConsoleHome::Accept).empty());
    // Persistent manual pin ordering; both rows and home use pinned_ order.
    const fs::path orderedRoot=root/"OrderedPinFixture";
    fs::create_directories(orderedRoot/"Games"/"A");
    fs::create_directories(orderedRoot/"Games"/"B");
    const fs::path orderedA=orderedRoot/"Games"/"A"/"default.xex";
    const fs::path orderedB=orderedRoot/"Games"/"B"/"default.xex";
    std::ofstream(orderedA)<<"a";std::ofstream(orderedB)<<"b";
    {std::ofstream out(orderedRoot/"aurorax-profile.txt");
     out<<"K\t"<<orderedA.string()<<"\nK\t"<<orderedB.string()<<"\n";}
    const std::vector<std::string> orderedRoots(1,(orderedRoot/"Games").string());
    LegacyFixtureHome ordered(orderedRoot.string()+"/",orderedRoots);ordered.Load();
    ordered.Press(ConsoleHome::Accept);
    for(int i=0;i<8;++i)ordered.Press(ConsoleHome::Right);
    ordered.Press(ConsoleHome::Search); // move A down
    LegacyFixtureHome orderedReload(orderedRoot.string()+"/",orderedRoots);orderedReload.Load();
    orderedReload.Press(ConsoleHome::Accept);
    for(int i=0;i<8;++i)orderedReload.Press(ConsoleHome::Right);
    orderedReload.Press(ConsoleHome::Accept);
    assert(orderedReload.Press(ConsoleHome::Accept)==orderedB.string());

    // Queue two copies, pause, resume and complete from the system panel.
    const fs::path queueRoot=root/"QueueFixture";
    fs::create_directories(queueRoot/"UserFiles");
    {std::ofstream out(queueRoot/"a.bin",std::ios::binary);out.seekp(200000);out.put('a');}
    {std::ofstream out(queueRoot/"b.bin",std::ios::binary);out.seekp(200000);out.put('b');}
    LegacyFixtureHome queueMenu(queueRoot.string()+"/",noRoots);queueMenu.Load();
    queueMenu.Press(ConsoleHome::Right);queueMenu.Press(ConsoleHome::Down);
    queueMenu.Press(ConsoleHome::Accept); // Files; directories sort first
    queueMenu.Press(ConsoleHome::Down);queueMenu.Press(ConsoleHome::Favorite); // a.bin
    queueMenu.Press(ConsoleHome::Up);queueMenu.Press(ConsoleHome::Accept); // UserFiles
    queueMenu.Press(ConsoleHome::PageUp);queueMenu.Press(ConsoleHome::Accept);
    assert(queueMenu.TransferRunning());
    queueMenu.Press(ConsoleHome::Back);
    queueMenu.Press(ConsoleHome::Down);queueMenu.Press(ConsoleHome::Down);
    queueMenu.Press(ConsoleHome::Favorite); // b.bin
    queueMenu.Press(ConsoleHome::Up);queueMenu.Press(ConsoleHome::Up);
    queueMenu.Press(ConsoleHome::Accept);
    queueMenu.Press(ConsoleHome::PageUp);queueMenu.Press(ConsoleHome::Accept);
    queueMenu.Press(ConsoleHome::Back);queueMenu.Press(ConsoleHome::Back); // Home
    queueMenu.Press(ConsoleHome::Right);queueMenu.Press(ConsoleHome::Accept); // System
    for(int i=0;i<3;++i)queueMenu.Press(ConsoleHome::Right);
    queueMenu.Press(ConsoleHome::Accept); // Downloads
    queueMenu.Press(ConsoleHome::Accept); // pause
    for(int i=0;i<20;++i)assert(!queueMenu.Tick());
    assert(!fs::exists(queueRoot/"UserFiles"/"a.bin"));
    queueMenu.Press(ConsoleHome::Accept); // resume
    for(int i=0;i<50 && queueMenu.TransferRunning();++i)queueMenu.Tick();
    assert(!queueMenu.TransferRunning());
    assert(fs::file_size(queueRoot/"UserFiles"/"a.bin")==200001);
    assert(fs::file_size(queueRoot/"UserFiles"/"b.bin")==200001);
    // Scan paths selected in the native explorer, persisted separately.
    const fs::path scanUiRoot=root/"ScanUiFixture";
    fs::create_directories(scanUiRoot/"CustomGames"/"Example");
    const fs::path scanUiExe=scanUiRoot/"CustomGames"/"Example"/"default.xex";
    std::ofstream(scanUiExe)<<"fixture";
    const fs::path scanUiConfig=scanUiRoot/"aurorax-scanpaths-ui.txt";
    LegacyFixtureHome scanUi(scanUiRoot.string()+"/",noRoots);scanUi.Load();
    scanUi.Press(ConsoleHome::Right);scanUi.Press(ConsoleHome::Down);
    scanUi.Press(ConsoleHome::Accept); // Files
    scanUi.Press(ConsoleHome::Accept); // CustomGames
    scanUi.Press(ConsoleHome::Left);scanUi.Press(ConsoleHome::Back); // cancel
    assert(!fs::exists(scanUiConfig));
    scanUi.Press(ConsoleHome::Left);scanUi.Press(ConsoleHome::Accept);
    assert(fs::exists(scanUiConfig));
    LegacyFixtureHome scanUiReload(scanUiRoot.string()+"/",noRoots);scanUiReload.Load();
    scanUiReload.Press(ConsoleHome::Accept);scanUiReload.Press(ConsoleHome::Accept);
    assert(scanUiReload.Press(ConsoleHome::Accept)==scanUiExe.string());
    scanUi.Press(ConsoleHome::Left);scanUi.Press(ConsoleHome::Accept); // remove
    assert(fs::file_size(scanUiConfig)==0);
    assert(fs::exists(scanUiExe));
    // Native explorer context actions: confirmed deletion, cancel and cut/paste.
    const fs::path actionRoot=root/"FileActionsFixture";
    fs::create_directories(actionRoot/"UserFiles"/"Dest");
    std::ofstream(actionRoot/"UserFiles"/"test.txt")<<"personal";
    LegacyFixtureHome actions(actionRoot.string()+"/",noRoots);actions.Load();
    actions.Press(ConsoleHome::Right);actions.Press(ConsoleHome::Down);
    actions.Press(ConsoleHome::Accept);actions.Press(ConsoleHome::Accept); // Files/UserFiles
    actions.Press(ConsoleHome::Down); // test.txt
    actions.Press(ConsoleHome::Right);actions.Press(ConsoleHome::Down);
    actions.Press(ConsoleHome::Accept);actions.Press(ConsoleHome::Back); // cancel deletion
    assert(fs::exists(actionRoot/"UserFiles"/"test.txt"));
    actions.Press(ConsoleHome::Right);actions.Press(ConsoleHome::Accept); // cut
    actions.Press(ConsoleHome::Up);actions.Press(ConsoleHome::Accept); // Dest
    actions.Press(ConsoleHome::PageUp);actions.Press(ConsoleHome::Accept); // move
    assert(!fs::exists(actionRoot/"UserFiles"/"test.txt"));
    assert(fs::exists(actionRoot/"UserFiles"/"Dest"/"test.txt"));
    actions.Press(ConsoleHome::Right);actions.Press(ConsoleHome::Down);
    actions.Press(ConsoleHome::Accept);actions.Press(ConsoleHome::Accept); // delete
    assert(!fs::exists(actionRoot/"UserFiles"/"Dest"/"test.txt"));
    // Multi-selection copies a directory tree and a separate file.
    const fs::path batchRoot=root/"BatchFixture";
    fs::create_directories(batchRoot/"Bundle"/"Empty");
    fs::create_directories(batchRoot/"UserFiles");
    std::ofstream(batchRoot/"Bundle"/"nested.txt")<<"nested";
    std::ofstream(batchRoot/"single.txt")<<"single";
    LegacyFixtureHome batch(batchRoot.string()+"/",noRoots);batch.Load();
    batch.Press(ConsoleHome::Right);batch.Press(ConsoleHome::Down);
    batch.Press(ConsoleHome::Accept); // Bundle, UserFiles, single.txt
    batch.Press(ConsoleHome::Right);batch.Press(ConsoleHome::Down);batch.Press(ConsoleHome::Down);
    batch.Press(ConsoleHome::Accept); // mark Bundle, enable multi-select
    batch.Press(ConsoleHome::Down);batch.Press(ConsoleHome::Down);
    batch.Press(ConsoleHome::Favorite); // mark single.txt
    batch.Press(ConsoleHome::Up);batch.Press(ConsoleHome::Accept); // UserFiles
    batch.Press(ConsoleHome::PageUp);batch.Press(ConsoleHome::Accept);
    for(int i=0;i<30 && batch.TransferRunning();++i)batch.Tick();
    assert(!batch.TransferRunning());
    assert(fs::is_directory(batchRoot/"UserFiles"/"Bundle"/"Empty"));
    assert(fs::file_size(batchRoot/"UserFiles"/"Bundle"/"nested.txt")==6);
    assert(fs::file_size(batchRoot/"UserFiles"/"single.txt")==6);
    assert(fs::exists(batchRoot/"Bundle"/"nested.txt"));
    const fs::path storeRoot=root/"StoreFixture";
    fs::create_directories(storeRoot);
    const std::string storeHash(64,'a');
    {std::ofstream out(storeRoot/"aurorax-store.tsv");
     out<<"demo\thomebrew\tExample\t1.0\t4\t"<<storeHash<<"\thttps://example.org/demo.zip\tAuthorized fixture\n";}
    LegacyFixtureHome storeMenu(storeRoot.string()+"/",noRoots);storeMenu.Load();
    storeMenu.Press(ConsoleHome::PageDown);storeMenu.Draw(canvas,"20:49");
    storeMenu.Press(ConsoleHome::Accept);storeMenu.Press(ConsoleHome::Accept);
    storeMenu.Press(ConsoleHome::Back); // cancel
    assert(!fs::exists(storeRoot/"aurorax-store-requests.tsv"));
    storeMenu.Press(ConsoleHome::Accept);storeMenu.Press(ConsoleHome::Accept);
    {std::ifstream in(storeRoot/"aurorax-store-requests.tsv");std::string line;
     std::getline(in,line);assert(line=="demo\t"+storeHash);}
    storeMenu.Press(ConsoleHome::Favorite);
    assert(fs::file_size(storeRoot/"aurorax-store-requests.tsv")==0);
    storeMenu.Press(ConsoleHome::Back);storeMenu.Press(ConsoleHome::Back);
    // PT-BR requests require a matching installed extracted XEX.
    const fs::path ptbrRoot=root/"TranslationFixture";
    const fs::path ptbrGame=ptbrRoot/"Games"/"Demo"/"default.xex";
    fs::create_directories(ptbrGame.parent_path());
    {std::ofstream out(ptbrGame,std::ios::binary);
     out.write(reinterpret_cast<const char*>(trainerGameHeader.data()),trainerGameHeader.size());}
    {std::ofstream out(ptbrRoot/"aurorax-store.tsv");
     out<<"ptbr\ttranslation\tTraducao PT-BR\t1\t4\t"<<storeHash
        <<"\thttps://example.org/ptbr.zip\tAuthorized\t4D5307E6\t12345678\t20010002\n";
     out<<"wrong\ttranslation\tOutra versao\t1\t4\t"<<storeHash
        <<"\thttps://example.org/other.zip\tAuthorized\t4D5307E6\t12345678\t20010003\n";}
    LegacyFixtureHome ptbr(ptbrRoot.string()+"/",std::vector<std::string>(1,(ptbrRoot/"Games").string()));
    ptbr.Load();ptbr.Press(ConsoleHome::PageDown);
    for(int i=0;i<7;++i)ptbr.Press(ConsoleHome::Right);
    ptbr.Press(ConsoleHome::Accept);ptbr.Press(ConsoleHome::Accept);ptbr.Press(ConsoleHome::Accept);
    assert(fs::exists(ptbrRoot/"aurorax-store-requests.tsv"));
    ptbr.Press(ConsoleHome::Back);ptbr.Press(ConsoleHome::Down);
    ptbr.Press(ConsoleHome::Accept);ptbr.Press(ConsoleHome::Accept);ptbr.Press(ConsoleHome::Accept);
    {std::ifstream in(ptbrRoot/"aurorax-store-requests.tsv");std::string line;
     std::getline(in,line);assert(line=="ptbr\t"+storeHash);assert(!std::getline(in,line));}
    // Nexus metadata only maps locally verified Title IDs; Aurora has precedence.
    const fs::path nexusRoot=root/"NexusMetadataFixture";
    const fs::path nexusGame=nexusRoot/"Games"/"Demo"/"default.xex";
    fs::create_directories(nexusGame.parent_path());
    {std::ofstream out(nexusGame,std::ios::binary);
     out.write(reinterpret_cast<const char*>(trainerGameHeader.data()),trainerGameHeader.size());}
    {std::ofstream out(nexusRoot/"aurorax-nexus-names.tsv");out<<"4D5307E6\tNexus title\n";}
    {std::ofstream out(nexusRoot/"aurorax-aurora-names.tsv");out<<"4D5307E6\tAurora title\n";}
    {std::ofstream out(nexusRoot/"aurorax-nexus-translation-ids.txt");out<<"4D5307E6\n";}
    LegacyFixtureHome nexusMeta(nexusRoot.string()+"/",std::vector<std::string>(1,(nexusRoot/"Games").string()));
    nexusMeta.Load();nexusMeta.Press(ConsoleHome::Accept);nexusMeta.Press(ConsoleHome::Accept);
    nexusMeta.Draw(canvas,"20:50");
    assert(nexusMeta.Press(ConsoleHome::Accept)==nexusGame.string());
    // Verify BMP byte order and exclusive publication using actual canvas pixels.
    {
        const auto shots=root/"ScreenFixture";
        fs::create_directories(shots/"UserFiles");
        const auto shotRoot=shots.string()+"/";
        FileWorkspace ws(shotRoot);HomeCanvas shot;
        shot.Box(0,0,1280,720,0xff123456u);
        std::vector<unsigned char> bytes;
        assert(DashboardScreenshot::Encode(shot.Pixels(),[&](const unsigned char* p,size_t n){bytes.insert(bytes.end(),p,p+n);return true;}));
        assert(bytes.size()==DashboardScreenshot::FileBytes&&bytes[0]=='B'&&bytes[1]=='M'&&bytes[28]==24);
        assert(bytes[54]==0x56&&bytes[55]==0x34&&bytes[56]==0x12);
        assert(!DashboardScreenshot::Encode(shot.Pixels(),[](const unsigned char*,size_t){return false;}));
#if defined(__linux__)
        const auto first=DashboardScreenshot::Save(shot,ws,shotRoot);
        const auto second=DashboardScreenshot::Save(shot,ws,shotRoot);
        assert(!first.empty()&&!second.empty()&&first!=second);
        assert(fs::file_size(first)==DashboardScreenshot::FileBytes&&fs::file_size(second)==DashboardScreenshot::FileBytes);
        assert(!fs::exists(first+".part")&&!fs::exists(second+".part"));
#endif
    }
    {
        unsigned lang=0;assert(UiLanguage::Parse("2",lang)&&lang==2);
        assert(!UiLanguage::Parse("3",lang)&&lang==2);
        assert(std::string(UiLanguage::Translate(1,"Biblioteca"))=="Library");
        assert(std::string(UiLanguage::Translate(2,"Arquivos"))=="Archivos");
        assert(std::string(UiLanguage::Translate(9,"Biblioteca"))=="Biblioteca");
        assert(std::string(UiLanguage::Translate(1,"game:/Games/Biblioteca/default.xex"))=="game:/Games/Biblioteca/default.xex");
    }
    {
        NotificationCenter center;
        const auto path=(root/"notification-test.log").string();
        {std::ofstream out(path);out<<"2026-11-25 21:00Z | FTP iniciado\n";}
        assert(center.Load(path)&&center.UnreadCount()==0);
        assert(center.Append("2026-11-25 21:01Z | Download nativo verificado"));
        assert(center.Append("2026-11-25 21:02Z | Backup recusado"));
        assert(center.UnreadCount()==2&&center.Visible(NotificationCenter::Attention).size()==1);
        assert(center.Mark(center.Visible(NotificationCenter::Unread).front()));
        assert(center.Save(path));
        NotificationCenter restored;assert(restored.Load(path)&&restored.UnreadCount()==1);
        restored.MarkAll();assert(restored.UnreadCount()==0);
        for(unsigned i=0;i<100;++i)assert(restored.Append("Copia agendada"));
        assert(restored.Visible(NotificationCenter::All).size()==64);
        assert(!restored.Append("bad\tdata"));
        {std::ofstream out(path);out<<std::string(32769,'x');}
        assert(!restored.Load(path)&&!restored.Save(path));
    }
    {
        assert(QuickGuide::Valid("01234567")&&!QuickGuide::Valid("11234567")&&!QuickGuide::Valid("01234568"));
        auto draft=std::string(QuickGuide::Default());int selected=0;
        assert(!QuickGuide::Move(draft,selected,-1));assert(QuickGuide::Move(draft,selected,1));
        assert(draft=="10234567"&&selected==1);
        const auto guideRoot=root/"QuickGuideFixture";fs::create_directories(guideRoot);
        const auto dir=guideRoot.string()+"/";
        LegacyFixtureHome guide(dir,{});guide.Load();guide.Press(ConsoleHome::Search);
        assert(guide.QuickGuideOpen()&&!guide.IdleEligible());
        guide.Press(ConsoleHome::Favorite);guide.Press(ConsoleHome::PageDown);guide.Press(ConsoleHome::Back);
        assert(guide.QuickGuideOrder()=="01234567");
        guide.Press(ConsoleHome::Favorite);guide.Press(ConsoleHome::PageDown);guide.Press(ConsoleHome::Refresh);
        assert(guide.QuickGuideOrder()=="10234567");
        LegacyFixtureHome reloaded(dir,{});reloaded.Load();assert(reloaded.QuickGuideOrder()=="10234567");
        reloaded.Press(ConsoleHome::Search);assert(reloaded.Press(ConsoleHome::Accept).empty()&&!reloaded.QuickGuideOpen());
        std::ofstream(guideRoot/"aurorax-profile.txt")<<"Q\t00000000\n";
        LegacyFixtureHome invalid(dir,{});invalid.Load();assert(invalid.QuickGuideOrder()==QuickGuide::Default());
    }
    assert(std::string(ConsoleStatus::StateLabel(false,0))=="Provider indisponivel");
    assert(std::string(ConsoleStatus::StateLabel(true,0))=="Sem sessao");
    assert(std::string(ConsoleStatus::StateLabel(true,1))=="Sessao local");
    assert(std::string(ConsoleStatus::StateLabel(true,99))=="Estado desconhecido");
    // Each palette preserves opaque alpha, warnings and media pixels.
    {
        HomeCanvas palette;
        unsigned accents[HomeCanvas::ThemeCount]={};
        for(unsigned t=0;t<HomeCanvas::ThemeCount;++t){
            palette.SetTheme(t);palette.Box(0,0,1,1,0xff9dec31u);accents[t]=palette.Pixels()[0];
            assert((accents[t]>>24)==255);
            palette.Box(1,0,1,1,0xffb83c28u);assert(palette.Pixels()[1]==0xffb83c28u);
            assert(std::string(HomeCanvas::ThemeName(t)).size()>0);
            for(unsigned k=0;k<t;++k)assert(accents[k]!=accents[t]);
        }
        palette.SetTheme(999);palette.Box(0,0,1,1,0xff9dec31u);assert(palette.Pixels()[0]==0xff9dec31u);
        const unsigned char video[]={0,255,0};palette.SetTheme(4);palette.VideoRgb(video,1,1);
        assert(palette.Pixels()[360*1280+640]==0xff00ff00u);
    }
    {
        unsigned p=0;assert(GameGrid::Parse("2",p)&&GameGrid::Enabled(p,0));
        assert(!GameGrid::Parse("3",p)&&p==2);
        assert(!GameGrid::Enabled(GameGrid::List,1));
        assert(!GameGrid::Enabled(GameGrid::Auto,0)&&GameGrid::Enabled(GameGrid::Auto,1));
        assert(GameGrid::Move(0,13,1)==1&&GameGrid::Move(5,13,1)==5);
        assert(GameGrid::Move(5,13,3)==11&&GameGrid::Move(11,13,3)==12);
        assert(GameGrid::Move(12,13,2)==6&&GameGrid::Move(12,13,1)==12);
        assert(GameGrid::Move(0,0,3)==0);
    }
    // Layout geometry: each destination is within the TV safe canvas and
    // reachable from every other destination using the directional model.
    {
        for(unsigned style=1;style<DashboardLayout::Count;++style){
            const auto g=DashboardLayout::Get(style);
            for(const auto& a:g.nav)assert(a.x>=0&&a.y>=0&&a.w>0&&a.h>0&&a.x+a.w<=1280&&a.y+a.h<=628);
            for(int start=0;start<DashboardLayout::Slots;++start){
                unsigned visited=1u<<start;
                for(int pass=0;pass<DashboardLayout::Slots;++pass)
                    for(int i=0;i<DashboardLayout::Slots;++i)if(visited&(1u<<i))
                        for(int d=0;d<4;++d)visited|=1u<<DashboardLayout::Move(g,i,d);
                assert(visited==(1u<<DashboardLayout::Slots)-1);
            }
        }
        unsigned mode=0;assert(DashboardLayout::Parse("9",mode)&&mode==9);
        assert(!DashboardLayout::Parse("10",mode)&&mode==9);
    }
    // Clock settings and bounded weather parser: no network dependency.
    {
        ClockWeather::Settings z;assert(ClockWeather::Settings::Parse("-180:1",z));assert(z.Encode()=="-180:1");
        assert(!ClockWeather::Settings::Parse("900:1",z)&&z.offset==-180);
        assert(!ClockWeather::Settings::Parse("-181:1",z));assert(!ClockWeather::Settings::Parse("0:999",z));
        assert(ClockWeather::Zone(-180)=="UTC-03:00");assert(ClockWeather::Time(1704067200,-180)=="21:00");
        assert(ClockWeather::Date(1704067200,-180)=="31/12/2023");
        ClockWeather::Reading wx;assert(ClockWeather::ParseReading(R"({"current":{"temperature_2m":24.2,"weather_code":2,"is_day":1,"time":1704067200}})",1,wx));
        assert(wx.valid&&wx.city==1&&wx.Fresh(1704069000)&&!wx.Fresh(1704075000));
        assert(ClockWeather::ParseReading(R"({"utc_offset_seconds":-10800,"current":{"temperature_2m":24.2,"weather_code":2,"is_day":1,"time":1704067200},"daily":{"time":[1704078000,1704164400],"temperature_2m_min":[18,19],"temperature_2m_max":[28,29],"weather_code":[2,61],"precipitation_probability_max":[null,80]}})",1,wx));
        assert(wx.dayCount==2&&wx.days[0].rain==-1&&wx.days[1].rain==80&&wx.cityOffsetSeconds==-10800);
        assert(ClockWeather::ParseReading(R"({"utc_offset_seconds":0,"current":{"temperature_2m":24.2,"weather_code":2,"is_day":1,"time":1704067200},"daily":{"time":[1704078000],"temperature_2m_min":[29],"temperature_2m_max":[18],"weather_code":[2],"precipitation_probability_max":[80]}})",1,wx)&&wx.dayCount==0);
        assert(!ClockWeather::ParseReading(R"({"current":{"temperature_2m":null,"weather_code":2,"is_day":1,"time":1704067200}})",1,wx));
        assert(!ClockWeather::ParseReading(R"({"current":{"temperature_2m":20,"weather_code":999,"is_day":1,"time":1704067200}})",1,wx));
    }
    // Appearance: invalid profile values leave the old preferences intact.
    {
        AppearanceSettings a;
        assert(a.Encode()=="0:1:0"&&a.ShowCover()&&a.ShowBackdrop());
        assert(AppearanceSettings::Parse("2:0:3",a)&&!a.ShowCover()&&!a.ShowBackdrop());
        assert(!AppearanceSettings::Parse("2:3:0",a)&&a.Encode()=="2:0:3");
        HomeCanvas canvas;canvas.SetAppearance(a,"");canvas.Background();
        assert(canvas.Pixels()[0]==0xff031310u);
        assert(!canvas.WallpaperReady());
        a.background=AppearanceSettings::Solid;canvas.SetAppearance(a,"");canvas.Background();
        assert(canvas.Pixels()[0]==canvas.Pixels()[1280*720-1]);
    }
    // Safe area: zero-copy default and explicit border/scale.
    {
        unsigned p=0;assert(DisplaySafeArea::Parse("10",p)&&p==10);
        assert(!DisplaySafeArea::Parse("11",p)&&p==10);
        assert(!DisplaySafeArea::Parse("-1",p));
        std::vector<unsigned> pixels(1280*720,0xff123456u),buffer;
        assert(DisplaySafeArea::Apply(pixels.data(),0,buffer)==pixels.data());
        assert(DisplaySafeArea::Apply(pixels.data(),10,buffer)==buffer.data());
        assert(buffer[0]==0xff000000u&&buffer[72*1280+128]==0xff123456u);
        assert(buffer[360*1280+640]==0xff123456u&&buffer.back()==0xff000000u);
    }
    {
        // Minimal big-endian XDBF with a title record; malformed counts rejected.
        std::vector<uint8_t> data(84,0);
        auto put=[&](size_t p,uint32_t n){for(unsigned i=0;i<4;++i)data[p+i]=uint8_t(n>>(24-i*8));};
        std::memcpy(data.data(),"XDBF",4);put(4,0x10000);put(8,1);put(12,1);
        data[25]=4;put(30,0x4d5307e6);put(38,42);
        put(42,0x4d5307e6);put(46,10);put(50,3);put(54,1000);put(58,200);
        GpdAchievements::Summary summary;
        assert(GpdAchievements::Dashboard(data,summary)&&summary.titles.size()==1&&summary.earnedScore==200&&summary.earned==3);
        put(50,11);assert(!GpdAchievements::Dashboard(data,summary)&&summary.titles.empty());
        put(50,3);put(38,43);assert(!GpdAchievements::Dashboard(data,summary));
        assert(std::string(UiLanguage::Translate(1,"Desligar console"))=="Shut down console");
        assert(std::string(UiLanguage::Translate(2,"Senha"))=="Contraseña");
        assert(std::string(UiLanguage::Translate(1,"Conquistas Xbox | GPD offline"))=="Xbox achievements | Offline GPD");
        assert(std::string(UiLanguage::Translate(2,"hdd:/Content"))=="hdd:/Content");
        assert(std::string(UiLanguage::Translate(0,"Senha"))=="Senha");
        LaunchStatistics stats;
        assert(stats.Attempt("game:/Games/Test/default.xex",1795570920));
        assert(!stats.Failed("other")&&stats.Failed("game:/Games/Test/default.xex"));
        assert(!stats.Failed("game:/Games/Test/default.xex"));
        assert(stats.Attempt("game:/Games/Test/default.xex",0));
        auto entry=stats.Get("game:/Games/Test/default.xex");
        assert(entry.attempts==2&&entry.failed==1&&entry.last==0);
        std::ostringstream out;stats.Write(out);
        LaunchStatistics restored;const auto line=out.str();
        assert(restored.LoadLine(line.substr(2,line.size()-3)));
        assert(restored.Get("game:/Games/Test/default.xex").attempts==2);
        assert(!restored.LoadLine("1\t2\t0\tbad"));
        assert(!restored.LoadLine("1000001\t0\t0\tbad"));
        assert(!restored.LoadLine("1\t0\t0\tbad\tpath"));
    }
    {
        IdleScreen idle;unsigned minutes=0;
        assert(IdleScreen::Parse("3",minutes)&&minutes==3&&!IdleScreen::Parse("2",minutes));
        assert(IdleScreen::Next(0)==1&&IdleScreen::Next(10)==0);
        assert(!idle.Update(1000,1,true,false));
        assert(!idle.Update(60999,1,true,false));
        assert(idle.Update(61000,1,true,false)&&idle.Active());
        assert(idle.Update(62000,1,true,false)&&idle.Step()==1);
        assert(idle.Update(62001,1,true,true)&&!idle.Active());
        assert(!idle.Update(120000,1,false,false));
        assert(!idle.Update(120001,1,true,false));
        assert(idle.Update(180000,1,true,false)&&idle.Active());
        assert(idle.Update(100,1,true,false)&&!idle.Active());
    }
    {unsigned char smc[16]={0x07,0x80,60,0,70,0,65,0,40};
     auto t=ThermalTelemetry::Decode(smc,sizeof smc);assert(t.state==ThermalTelemetry::Valid&&t.Celsius(0)=="60.5 C");
     assert(t.CompactCelsius(0)=="61 C"&&t.CompactCelsius(5)=="--");
     assert(ThermalTelemetry::HeaderDue(false,0,0)&&!ThermalTelemetry::HeaderDue(true,1000,10999)&&ThermalTelemetry::HeaderDue(true,1000,11000)&&ThermalTelemetry::HeaderDue(true,1000,999));
     smc[3]=0;smc[4]=0;assert(ThermalTelemetry::Decode(smc,sizeof smc).state==ThermalTelemetry::Partial);
     assert(ThermalTelemetry::Decode(smc,8).state==ThermalTelemetry::Invalid);
     smc[0]=0;assert(ThermalTelemetry::Decode(smc,sizeof smc).state==ThermalTelemetry::Invalid);}
    {unsigned char reply[16]={0x07,0,60,0,80,0,65,0,40,73};
      auto sample=ThermalTelemetry::Decode(reply,sizeof reply);
      assert(sample.fanTargetAvailable&&sample.FanTargetLabel()=="73%");
      assert(ThermalAdvisory::Evaluate(sample,0)==ThermalAdvisory::Disabled);
      assert(ThermalAdvisory::Evaluate(sample,80)==ThermalAdvisory::Reached);
      assert(ThermalAdvisory::Evaluate(sample,85)==ThermalAdvisory::Below);
      reply[3]=reply[4]=0;sample=ThermalTelemetry::Decode(reply,sizeof reply);
      assert(ThermalAdvisory::Evaluate(sample,85)==ThermalAdvisory::Incomplete);
      assert(ThermalAdvisory::Evaluate(sample,60)==ThermalAdvisory::Reached);
      reply[9]=101;assert(!ThermalTelemetry::Decode(reply,sizeof reply).fanTargetAvailable);
      reply[9]=0;assert(!ThermalTelemetry::Decode(reply,sizeof reply).fanTargetAvailable);
      reply[0]=0;reply[9]=73;sample=ThermalTelemetry::Decode(reply,sizeof reply);
      assert(!sample.fanTargetAvailable&&ThermalAdvisory::Evaluate(sample,80)==ThermalAdvisory::NoReading);
      assert(ThermalAdvisory::Next(0,1)==60&&ThermalAdvisory::Next(95,1)==0);
      assert(ThermalAdvisory::Next(0,-1)==95&&ThermalAdvisory::Next(60,-1)==0);
      assert(!ThermalAdvisory::Valid(82)&&ThermalAdvisory::Next(82,1)==0);}
    {ThermalHistory history;
      unsigned char smc[16]={0x07,0x80,60,0,70,0,65,0,40};
      history.Add(ThermalTelemetry::Decode(smc,sizeof smc));
      assert(history.attempts==1&&history.valid==1&&history.Min(0)=="60.5 C"&&history.Max(0)=="60.5 C");
      smc[1]=0;smc[2]=55;smc[3]=0;smc[4]=0;smc[6]=80;
      history.Add(ThermalTelemetry::Decode(smc,sizeof smc));
      assert(history.attempts==2&&history.partial==1&&history.Min(0)=="55.0 C"&&history.Max(0)=="60.5 C");
      assert(history.Min(1)=="70.0 C"&&history.Max(2)=="80.0 C");
      smc[0]=0;history.Add(ThermalTelemetry::Decode(smc,sizeof smc));
      assert(history.invalid==1&&history.attempts==3&&history.Min(0)=="55.0 C");
      ThermalTelemetry unsupported;unsupported.state=ThermalTelemetry::Unsupported;history.Add(unsupported);
      assert(history.attempts==3&&history.Min(4)=="--");
      history.attempts=UINT32_MAX;history.Add(ThermalTelemetry::Decode(smc,sizeof smc));
      assert(history.attempts==UINT32_MAX);
      history.Clear();assert(history.attempts==0&&history.Min(0)=="--"&&history.Max(2)=="--");
      smc[0]=0x07;history.Add(ThermalTelemetry::Decode(smc,sizeof smc));
      assert(history.Min(1)=="--"&&history.Min(0)=="55.0 C");}
    {ConsolePower::Confirmation power;
     assert(!ConsolePower::Supported());
     assert(power.Accept(false,false)==ConsolePower::Unavailable&&!power.armed);
     assert(power.Accept(true,true)==ConsolePower::Busy&&!power.armed);
     assert(power.Accept(true,false)==ConsolePower::Armed&&power.armed);
     power.Select(ConsolePower::Reboot);assert(!power.armed);
     assert(power.Accept(true,false)==ConsolePower::Armed);
     assert(power.Accept(true,true)==ConsolePower::Busy&&!power.armed);
     assert(power.Accept(true,false)==ConsolePower::Armed);
     power.Cancel();assert(!power.armed);
     assert(power.Accept(true,false)==ConsolePower::Armed);
     assert(power.Accept(true,false)==ConsolePower::Requested&&!power.armed);
     assert(ConsolePower::Routine(ConsolePower::Shutdown)==5&&ConsolePower::Routine(ConsolePower::Reboot)==6);
     assert(ConsolePower::Invoke(ConsolePower::Shutdown)==ConsolePower::Unavailable);}
    {const auto req=SmbProbe::Request();
     assert(req.size()==130&&req[3]==126&&req[4]==0xfe&&SmbProbe::U16(req.data()+68)==36);
     assert(SmbProbe::U16(req.data()+70)==5&&SmbProbe::U16(req.data()+112)==0x0311);
     assert(SmbProbe::U32(req.data()+96)==112&&SmbProbe::U16(req.data()+116)==1&&SmbProbe::U16(req.data()+128)==1);
     unsigned char response[132]={0xfe,'S','M','B',64};
     response[16]=1;response[64]=65;response[66]=3;response[68]=2;response[69]=3;
     SmbProbe::Put16(response+120,128);SmbProbe::Put16(response+122,4);
     SmbProbe::Result result;
     assert(SmbProbe::Parse(response,sizeof response,result)==SmbProbe::Negotiated&&result.dialect==0x0302&&result.signingRequired);
     assert(SmbProbe::Parse(response,131,result)==SmbProbe::Failed);
     response[68]=0x11;assert(SmbProbe::Parse(response,sizeof response,result)==SmbProbe::Failed);
     response[8]=0xbb;assert(SmbProbe::Parse(response,64,result)==SmbProbe::Rejected&&result.status==0xbb);
     response[24]=1;assert(SmbProbe::Parse(response,64,result)==SmbProbe::Failed);
     SmbProbe probe;assert(!probe.Start("192.168.1.1","8.8.8.8")&&!probe.Running());
     unsigned char modern[150]={0xfe,'S','M','B',64};
     modern[16]=1;modern[64]=65;modern[66]=3;modern[68]=0x11;modern[69]=3;modern[70]=1;
     modern[120]=128;modern[122]=4;modern[124]=136;
     modern[136]=1;modern[138]=6;modern[144]=1;modern[148]=1;
     assert(SmbProbe::Parse(modern,sizeof modern,result)==SmbProbe::Negotiated&&result.preauthSha512&&result.securityTokenBytes==4);
     assert(SmbProbe::Parse(modern,149,result)==SmbProbe::Failed);
     modern[148]=2;assert(SmbProbe::Parse(modern,sizeof modern,result)==SmbProbe::Failed);
     modern[148]=1;modern[70]=2;assert(SmbProbe::Parse(modern,sizeof modern,result)==SmbProbe::Failed);
     modern[70]=1;modern[124]=128;assert(SmbProbe::Parse(modern,sizeof modern,result)==SmbProbe::Failed);
     modern[124]=136;modern[146]=1;assert(SmbProbe::Parse(modern,sizeof modern,result)==SmbProbe::Failed);
     modern[146]=0;modern[70]=0;assert(SmbProbe::Parse(modern,sizeof modern,result)==SmbProbe::Failed);}
    // Kernel counters remain separate; invalid or unavailable snapshots fail closed.
    const auto mem=SystemTelemetry::FromPages(131072,2048,16384,32768,8192,1024,128,512);
    assert(mem.available&&mem.physical==512ULL*1024*1024&&mem.titleAvailable==128ULL*1024*1024);
    assert(mem.titleHeap==4ULL*1024*1024&&mem.titleStack==512ULL*1024);
    assert(!SystemTelemetry::FromPages(0,0,0,0,0,0,0,0).available);
    assert(!SystemTelemetry::FromPages(100,101,0,0,0,0,0,0).available);
    assert(!SystemTelemetry::Read().available);
    auto video=VideoTelemetry::FromMode(1920,1080,59.94f,0,1,1,1);
    assert(video.available&&video.width==1920&&video.Frequency()=="59.940 Hz"&&video.widescreen&&!video.interlaced);
    video.SetDisplay(1920,1080,1280,720);
    assert(video.displayAvailable&&video.bufferWidth==1280);
    video.SetDisplay(1920,1080,0,0);assert(!video.displayAvailable&&video.bufferWidth==0);
    assert(!VideoTelemetry::FromMode(0,720,60,0,1,1,1).available);
    assert(!VideoTelemetry::FromMode(1280,720,0,0,1,1,1).available);
    assert(!VideoTelemetry::FromMode(1280,720,60,2,1,1,1).available);
    assert(!VideoTelemetry::Read().available);
    // Read-only INI inventory; duplicate slots are ambiguous, not loaded state.
    const fs::path residentRoot=root/"ResidentFixture";
    fs::create_directories(residentRoot);
    const fs::path ini=residentRoot/"launch.ini";
    const std::string iniText="[Paths]\nplugin1=ignored.xex\n[Plugins]\nplugin1 = Hdd1:\\LiveEmulation.xex\nplugin2=\nplugin3=one.xex\nplugin3=two.xex\n[Settings]\nplugin5=ignored.xex\n";
    {std::ofstream out(ini);out<<iniText;}
    std::vector<ResidentPluginSlot> slots;
    assert(readResidentPlugins(ini.string(),slots));
    assert(slots.size()==5 && isLiveEmulationReference(slots[0].path));
    assert(slots[1].path.empty() && slots[2].duplicate && slots[4].path.empty());
    const auto secondIni=residentRoot/"other.ini",badIni=residentRoot/"bad.ini";
    {std::ofstream out(secondIni);out<<"[Plugins]\nplugin1=hdd:/liveemulation.xex\nplugin2=HDD1:\\LIVEEMULATION.XEX\n";}
    {std::ofstream out(badIni);out<<std::string(65537,'x');}
    const auto configs=residentInventory({ini.string(),secondIni.string(),badIni.string(),ini.string(),(residentRoot/"missing.ini").string()});
    assert(configs.size()==3&&configs[0].valid&&configs[1].valid&&!configs[2].valid);
    assert(configs[1].slots[0].repeated&&configs[1].slots[1].repeated);
    assert(!residentConflict(configs,0,0)&&residentConflict(configs,0,1)&&residentConflict(configs,0,2));
    assert(!residentConflict(configs,2,0));
    
    LegacyFixtureHome residentMenu(residentRoot.string()+"/",noRoots);residentMenu.Load();
    residentMenu.Press(ConsoleHome::Right);residentMenu.Press(ConsoleHome::Down);
    residentMenu.Press(ConsoleHome::Right);residentMenu.Press(ConsoleHome::Accept);
    for(int i=0;i<4;++i)residentMenu.Press(ConsoleHome::Right);
    residentMenu.Press(ConsoleHome::Accept);residentMenu.Draw(canvas,"21:00");
    residentMenu.Press(ConsoleHome::PageDown);residentMenu.Draw(canvas,"21:00");
    residentMenu.Press(ConsoleHome::Refresh);
    {std::ifstream in(ini);std::string contents((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());assert(contents==iniText);}
    // Receipts are snapshots bound to the selected catalog hash and size.
    fs::create_directories(storeRoot/"UserFiles"/"Store");
    StoreCatalog receiptCatalog(storeRoot.string()+"/");
    assert(receiptCatalog.Load());
    const StoreItem receiptItem=receiptCatalog.Items()[0];
    const fs::path receiptPath=storeRoot/"UserFiles"/"Store"/"demo.state";
    {std::ofstream out(receiptPath);out<<storeHash<<"\treceiving\t2\n";}
    assert(receiptCatalog.Progress(receiptItem).state==StoreProgress::Receiving);
    assert(receiptCatalog.Progress(receiptItem).received==2);
    {std::ofstream out(receiptPath);out<<storeHash<<"\tverified\t4\n";}
    assert(receiptCatalog.Progress(receiptItem).state==StoreProgress::Unknown);
    {std::ofstream out(storeRoot/"UserFiles"/"Store"/"demo.pkg");out<<"data";}
    assert(receiptCatalog.Progress(receiptItem).state==StoreProgress::Verified);
    {std::ofstream out(receiptPath);out<<std::string(64,'b')<<"\tverified\t4\n";}
    assert(receiptCatalog.Progress(receiptItem).state==StoreProgress::Unknown);
    assert(fs::exists(storeRoot/"infinity360-events.log"));
    {std::ifstream in(storeRoot/"infinity360-events.log");std::string log((std::istreambuf_iterator<char>(in)),std::istreambuf_iterator<char>());assert(log.find("Pedido da loja salvo")!=std::string::npos);}
    fs::remove_all(root);
}
