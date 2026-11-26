#include "ConsoleHome.h"
#if defined(__XSDK__)
#define INFINITY_SOCIAL_XSDK 1
#include "SocialProfileXsdk.h"
#endif
#include "DashboardLayout.h"
#include "GameGrid.h"
#include "ApprovedHome.h"
#include "InfinityIcons.h"
#include "ControllerButtons.h"
#include "StoreSourceDirectory.h"
#include "InfinityVersion.h"
#include "DiscIdentity.h"
#include "ArtImport.h"
#include "StoreStorage.h"
#include "XsdkPaths.h"
#include "SavedFile.h"
#include "PluginRegistry.h"
#include "ContentCatalog.h"
#include <ctime>
#include <cctype>
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <set>

namespace {
const size_t HistoryLimit=24;
const size_t PinLimit=32;
const size_t ArtChoiceLimit=512;
// Import only names keyed by a locally verified Title ID. The exporter never
// supplies a launch target or bypasses the local scanner.
std::map<unsigned,std::string> importedNames(const std::string& file) {
    std::map<unsigned,std::string> names;
    std::ifstream in(file.c_str(),std::ios::binary);
    if(!in)return names;
    const size_t limit=1024*1024;
    std::vector<char> data(limit+1);
    in.read(&data[0],static_cast<std::streamsize>(data.size()));
    if(in.bad() || in.gcount()>static_cast<std::streamsize>(limit))return names;
    std::istringstream lines(std::string(&data[0],static_cast<size_t>(in.gcount())));
    std::string line;
    while(names.size()<8192 && std::getline(lines,line)) {
        if(!line.empty() && line[line.size()-1]=='\r')line.erase(line.size()-1);
        if(line.size()<10 || line.size()>249 || line[8]!='\t')continue;
        unsigned id=0;bool valid=true;
        for(size_t i=0;i<8;++i) {
            const char ch=line[i];
            if(ch>='0' && ch<='9')id=(id<<4)|(ch-'0');
            else if(ch>='A' && ch<='F')id=(id<<4)|(ch-'A'+10);
            else {valid=false;break;}
        }
        for(size_t i=9;i<line.size();++i)
            if(static_cast<unsigned char>(line[i])<32 || line[i]==127)valid=false;
        if(valid && id && !names.count(id))names[id]=line.substr(9);
    }
    return names;
}
std::set<unsigned> importedTranslationIds(const std::string& file) {
    std::set<unsigned> ids;
    std::ifstream in(file.c_str(),std::ios::binary);
    if(!in)return ids;
    const size_t cap=128*1024;
    std::vector<char> raw(cap+1);
    in.read(&raw[0],static_cast<std::streamsize>(raw.size()));
    if(in.bad()||in.gcount()>static_cast<std::streamsize>(cap))return ids;
    std::istringstream lines(std::string(&raw[0],static_cast<size_t>(in.gcount())));
    std::string line;
    while(std::getline(lines,line)) {
        if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
        if(line.size()!=8)continue;
        unsigned id=0;bool valid=true;
        for(size_t i=0;i<line.size();++i) {
            const char ch=line[i];
            if(ch>='0'&&ch<='9')id=(id<<4)|(ch-'0');
            else if(ch>='A'&&ch<='F')id=(id<<4)|(ch-'A'+10);
            else {valid=false;break;}
        }
        if(valid&&id)ids.insert(id);
        if(ids.size()>=8192)break;
    }
    return ids;
}
bool validArtChoice(const std::string& value) {
    return value.size()==2 && value[0]>='0' && value[0]<='2' &&
                              value[1]>='0' && value[1]<='2';
}
bool validArtId(const std::string& id) {
    if(id.size()!=8 || id=="00000000")return false;
    for(size_t i=0;i<id.size();++i)
        if(!((id[i]>='0' && id[i]<='9') || (id[i]>='A' && id[i]<='F')))return false;
    return true;
}
std::string artFile(const std::string& folder,bool cover,char choice) {
    if(folder.empty() || choice=='2')return "";
    return folder+(cover?(choice=='1'?"cover-alt.rgba":"cover.rgba"):
                          (choice=='1'?"background-alt.rgba":"background.rgba"));
}
const char DefaultHomeOrder[]="0123";
bool validHomeOrder(const std::string& order) {
    if(order.size()!=4) return false;
    for(char ch='0';ch<='3';++ch)
        if(std::count(order.begin(),order.end(),ch)!=1) return false;
    return true;
}
const char* const labels[]={"Biblioteca","Cloud360 IPTV","Aplicativos","Emuladores","Arquivos","Sistema","Recentes","Fixados e favoritos"};
std::string number(size_t value) { std::ostringstream out; out<<value; return out.str(); }
bool contains(const std::vector<std::string>& list,const std::string& path) {
    return std::find(list.begin(),list.end(),path)!=list.end();
}
void clampRow(int& row,size_t count) {
    row = count ? std::min(row,static_cast<int>(count-1)) : 0;
}
struct LibraryOrder {
    explicit LibraryOrder(int value):value_(value) {}
    bool operator()(const Game* a,const Game* b) const {
        if(value_==1) return a->name>b->name;
        if(value_==2 && a->device!=b->device) return a->device<b->device;
        return a->name<b->name;
    }
    int value_;
};
// Classify extracted XEX and read-only STFS/GOD packages in separate tabs.
// A package is never treated as launchable by XLaunchNewImage.
bool matchesLibraryQuery(const Game& item,const std::string& query) {
    if(query.empty()) return true;
    std::string text=item.name;
    for(size_t i=0;i<text.size();++i)
        text[i]=(char)std::tolower((unsigned char)text[i]);
    std::string needle=query;
    for(size_t i=0;i<needle.size();++i)
        needle[i]=(char)std::tolower((unsigned char)needle[i]);
    return text.find(needle)!=std::string::npos;
}
const char* const keyboardKeys[]={
    "A","B","C","D","E","F","G",
    "H","I","J","K","L","M","N",
    "O","P","Q","R","S","T","U",
    "V","W","X","Y","Z","0","1",
    "2","3","4","5","6","7","8",
    "9","ESP","APAGAR","BUSCAR","LIMPAR","CANCELAR","."
};
const int KeyCount=42;
bool isXexPath(const std::string& path) {
    if(path.size()<4) return false;
    std::string tail=path.substr(path.size()-4);
    for(size_t i=0;i<tail.size();++i)
        tail[i]=(char)std::tolower((unsigned char)tail[i]);
    return tail==".xex";
}
bool isContentPackage(const Game& item) {
    return item.device.compare(0,7,"Pacote ")==0;
}
bool libraryTabMatch(const Game& item,int tab) {
    if(tab==0) return true;
    if(tab==4) return isContentPackage(item);
    if(isContentPackage(item)) {
        if(tab==2) return item.device=="Pacote XBLA";
        if(tab==3) return item.device=="Pacote Xbox Original";
        if(tab==1) return item.device!="Pacote XBLA" &&
                          item.device!="Pacote Xbox Original";
        return false;
    }
    std::string name=item.executable;
    for(size_t i=0;i<name.size();++i) {
        if(name[i]=='\\') name[i]='/';
        name[i]=(char)std::tolower((unsigned char)name[i]);
    }
    const bool arcade=name.find("/xbla/")!=std::string::npos;
    const bool original=name.find("/xbox original/")!=std::string::npos;
    return tab==1?(!arcade && !original):tab==2?arcade:original;
}
}

ConsoleHome::ConsoleHome(const std::string& dataDir,const std::vector<std::string>& roots,unsigned initialLayout)
    : dir_(dataDir),roots_(roots),baseRoots_(roots),tv_(dataDir),
      collections_(dataDir+"aurorax-collections.txt"),
      fileWorkspace_(dataDir),ftp_(fileWorkspace_),copyPaused_(false),storeDownloads_(false),storeDownloadRow_(0),storagePage_(0),fileRoot_(0),pasteConfirm_(false),scanPathConfirm_(false),fileAction_(false),deleteConfirm_(false),cutMode_(false),fileActionRow_(0),multiSelect_(false),
      focus_(0),row_(0),subtab_(0),systemTab_(0),gameSort_(0),searchCursor_(0),
      homeOrder_(DefaultHomeOrder),homeDraft_(DefaultHomeOrder),homeEditSlot_(0),homeEditing_(false),storeActive_(false),storeDetails_(false),storeConfirm_(false),storeTab_(0),storeRow_(0),store_(dataDir),storeTranslationMatch_(false),nativeStorePending_(false),importPreview_(false),
      collectionIndex_(0),collectionRow_(0),keyboardMode_(0),
      searchActive_(false),collectionPicker_(false),deleteCollectionConfirm_(false),
      contentActive_(false),contentKnown_(false),contentTab_(0),contentRow_(0),
      contentInfo_(),artDirty_(true),artEditing_(false),artEditRow_(0),trainerConfirm_(false),section_(Library),page_(Home) {initialLayout_=initialLayout<DashboardLayout::Count?initialLayout:DashboardLayout::TopIcons;layout_=layoutDraft_=initialLayout_;}

void ConsoleHome::Load() {
    LoadEvents();
    LoadScanPaths();
    if(!collections_.Load())notice_="Colecoes invalidas: edicao bloqueada para preservar o arquivo.";
    const std::string path=dir_+"aurorax-profile.txt";
    const std::string src=savedFile(path);
    std::ifstream in(src.c_str());
    history_.clear(); favorites_.clear(); pinned_.clear(); hidden_.clear(); launched_.clear();launchStats_.Clear();
    homeOrder_=DefaultHomeOrder;homeEditing_=false;approvedPage_=approvedHero_=0;approvedHeroAt_=0;approvedReversePath_.clear();approvedGroup_=-1;approvedGroupRow_=0;approvedShortcuts_="012345";approvedDraft_.clear();theme_=0;safeArea_=0;idleMinutes_=0;language_=0;
    appearance_={};appearanceDraft_={};appearanceEditor_=false;wallpaperDirty_=true;layout_=layoutDraft_=initialLayout_;layoutPreview_=false;focus_=0;gridPreference_=gridDraft_=0;gridFilter_=false;
    clockSettings_={};clockDraft_={};weather_={};weatherPending_=false;weatherPanel_=false;lastWeatherAttempt_=0;
    quickGuideOpen_=false;quickGuideEditing_=false;quickGuideRow_=0;socialGuideFocus_=false;socialGuideRow_=0;nativeSocialPending_=InfinitySocial::NativeAction::None;quickGuideOrder_=QuickGuide::Default();quickGuideDraft_.clear();
    artChoices_.clear();artEditing_=false;importPreview_=false;
    disabledPluginIds_.clear();localPlugins_=false;localPluginRow_=0;
    bool designSelected=false;
    std::string line;
    while (std::getline(in,line)) {
        if (!line.empty() && line[line.size()-1]=='\r') line.erase(line.size()-1);
        if (line.size()<3 || line.size()>2048 || line[1]!='\t') continue;
        const std::string file=line.substr(2);
        if(line[0]=='O' && validHomeOrder(file)) homeOrder_=file;
        if(line[0]=='U'&&ApprovedHome::ValidOrder(file))approvedShortcuts_=file;
        if(line[0]=='T'&&file.size()==1&&file[0]>='0'&&static_cast<unsigned>(file[0]-'0')<HomeCanvas::ThemeCount)theme_=file[0]-'0';
        if(line[0]=='W')AppearanceSettings::Parse(file,appearance_);
        if(line[0]=='J')DashboardLayout::Parse(file,layout_);
        if(line[0]=='M'&&file=="1")designSelected=true;
        if(line[0]=='Z')ClockWeather::Settings::Parse(file,clockSettings_);
        if(line[0]=='G')GameGrid::Parse(file,gridPreference_);
        if(line[0]=='V')DisplaySafeArea::Parse(file,safeArea_);
        if(line[0]=='I')IdleScreen::Parse(file,idleMinutes_);
        if(line[0]=='L')UiLanguage::Parse(file,language_);
        if(line[0]=='Q'&&QuickGuide::Valid(file))quickGuideOrder_=file;
        if(line[0]=='D'&&disabledPluginIds_.size()<256){const auto id=PluginRegistry::Key(file);if(!id.empty())disabledPluginIds_.insert(id);}
        if(line[0]=='A' && file.size()==11 && file[8]=='\t') {
            const std::string id=file.substr(0,8), choice=file.substr(9);
            if(validArtId(id) && validArtChoice(choice)) {
                if(choice=="00")artChoices_.erase(id);
                else if(artChoices_.size()<ArtChoiceLimit || artChoices_.count(id))
                    artChoices_[id]=choice;
            }
        }
        if(line[0]=='R' && history_.size()<HistoryLimit && !contains(history_,file)) history_.push_back(file);
        if(line[0]=='F' && !contains(favorites_,file)) favorites_.push_back(file);
        if(line[0]=='K' && pinned_.size()<PinLimit && !contains(pinned_,file)) pinned_.push_back(file);
        if(line[0]=='H' && !contains(hidden_,file)) hidden_.push_back(file);
        if(line[0]=='S')launchStats_.LoadLine(file);
        if(line[0]=='P' && launched_.size()<4096 && !contains(launched_,file))
            launched_.push_back(file);
    }
    // M marks a layout choice made after the approved design migration.
    if(initialLayout_==DashboardLayout::TopIcons&&!designSelected&&layout_==DashboardLayout::Classic)layout_=DashboardLayout::TopIcons;
    layoutDraft_=layout_;
    // Legacy profiles recorded only R; these were also launch requests.
    for(size_t i=0;i<history_.size() && launched_.size()<4096;++i)
        if(!contains(launched_,history_[i])) launched_.push_back(history_[i]);
    ScanLibrary();
    Rebuild();
    if(layout_==DashboardLayout::TopIcons){RefreshStorage();RefreshNetwork();status_=ConsoleStatus::Read();}
}
void ConsoleHome::ScanLibrary() {
    artFolders_.clear();artDirty_=true;
    games_=GameScanner(roots_).Scan();
    auto packageRoots=ContentCatalog::DefaultRoots(dir_);
    const auto managed=PackageImport::Roots(dir_);packageRoots.insert(packageRoots.end(),managed.begin(),managed.end());
    const std::vector<XboxContentPackage> packages=ContentCatalog(packageRoots).Scan();
    for(size_t i=0;i<packages.size();++i) {
        const XboxContentPackage& pkg=packages[i];
        std::string kind="Pacote Xbox 360";
        if(pkg.contentType==0x7000u) kind="Pacote GOD";
        else if(pkg.contentType==0xd0000u) kind="Pacote XBLA";
        else if(pkg.contentType==0x5000u) kind="Pacote Xbox Original";
        else if(pkg.contentType==0x2000000u) kind="Pacote Indie";
        else if(pkg.contentType==0x4000u) kind="Pacote Instalado";
        else if(pkg.contentType==0x80000u) kind="Pacote Demo";
        Game item={pkg.name,pkg.path,kind,false};
        games_.push_back(item);
    }
    std::map<unsigned,std::string> names=importedNames(dir_+"aurorax-nexus-names.tsv");
    const std::map<unsigned,std::string> aurora=importedNames(dir_+"aurorax-aurora-names.tsv");
    for(std::map<unsigned,std::string>::const_iterator it=aurora.begin();it!=aurora.end();++it)
        names[it->first]=it->second;
    translationReferences_=importedTranslationIds(dir_+"aurorax-nexus-translation-ids.txt");
    if(!names.empty()) {
        for(size_t i=0;i<games_.size();++i) {
            const XboxTitleMetadata info=ContentCatalog::DescribeTitle(games_[i].executable,isContentPackage(games_[i]));
            const std::map<unsigned,std::string>::const_iterator it=names.find(info.titleId);
            if(info.known && it!=names.end())games_[i].name=it->second;
        }
    }
}
bool ConsoleHome::SaveProfile() const {
    const std::string path=dir_+"aurorax-profile.txt", tmp=path+".tmp";
    std::ofstream out(tmp.c_str(),std::ios::trunc);
    out<<"O\t"<<homeOrder_<<"\n";
    out<<"U\t"<<approvedShortcuts_<<"\n";
    out<<"T\t"<<theme_<<"\n";
    out<<"W\t"<<appearance_.Encode()<<"\n";
    out<<"J\t"<<layout_<<"\n";
    out<<"M\t1\n";
    out<<"Z\t"<<clockSettings_.Encode()<<"\n";
    out<<"G\t"<<gridPreference_<<"\n";
    out<<"V\t"<<safeArea_<<"\n";
    out<<"I\t"<<idleMinutes_<<"\n";
    out<<"L\t"<<language_<<"\n";
    out<<"Q\t"<<quickGuideOrder_<<"\n";
    for(const auto& id:disabledPluginIds_)out<<"D\t"<<id<<"\n";
    launchStats_.Write(out);
    for(std::map<std::string,std::string>::const_iterator it=artChoices_.begin();
        it!=artChoices_.end();++it)
        out<<"A\t"<<it->first<<"\t"<<it->second<<"\n";
    for (size_t i=0;i<history_.size();++i) out<<"R\t"<<history_[i]<<"\n";
    for (size_t i=0;i<favorites_.size();++i) out<<"F\t"<<favorites_[i]<<"\n";
    for (size_t i=0;i<pinned_.size();++i) out<<"K\t"<<pinned_[i]<<"\n";
    for (size_t i=0;i<hidden_.size();++i) out<<"H\t"<<hidden_[i]<<"\n";
    for (size_t i=0;i<launched_.size();++i) out<<"P\t"<<launched_[i]<<"\n";
    out.close();
    if (!out) return false;
    return replaceSavedFile(path);
}
bool ConsoleHome::Starred(const Game& item) const { return contains(favorites_,item.executable); }
std::vector<std::string> ConsoleHome::HomeShortcuts() const {
    std::vector<std::string> paths=pinned_;
    for(size_t i=0;i<favorites_.size();++i)
        if(!contains(paths,favorites_[i])) paths.push_back(favorites_[i]);
    return paths;
}
const Game* ConsoleHome::Installed(const std::string& path) const {
    for(size_t i=0;i<games_.size();++i) if(games_[i].executable==path) return &games_[i];
    return 0;
}
size_t ConsoleHome::Count() const {
    if (section_==Files) return files_.size();
    if (section_==System) return 0;
    return visible_.size();
}
void ConsoleHome::Rebuild() {
    visible_.clear();
    if (section_==Recent || section_==Favorites) {
        const std::vector<std::string> paths = section_==Recent ? history_ : HomeShortcuts();
        for(size_t i=0;i<paths.size();++i) {
            const Game* g=Installed(paths[i]);
            if(g && !contains(hidden_,paths[i])) visible_.push_back(g);
        }
    } else if (section_==Library) {
        for(size_t i=0;i<games_.size();++i) {
            const bool hidden=contains(hidden_,games_[i].executable);
            const bool inCollection=collectionIndex_>=0 &&
                static_cast<size_t>(collectionIndex_)<collections_.Items().size() &&
                collections_.Contains(static_cast<size_t>(collectionIndex_),
                                      games_[i].executable);
            const bool neverOpened=!isContentPackage(games_[i]) &&
                !contains(launched_,games_[i].executable);
            const bool allowed=subtab_==7?hidden:
                 (!hidden && (subtab_==8?contains(pinned_,games_[i].executable):
                               subtab_==6?neverOpened:
                              subtab_==5?inCollection:
                              libraryTabMatch(games_[i],subtab_)));
            if(allowed && matchesLibraryQuery(games_[i],searchTerm_))
                visible_.push_back(&games_[i]);
        }
        if(subtab_==8) {
            // Pins are rendered in the same persisted order as the home cards.
            const std::vector<const Game*> filtered=visible_;
            visible_.clear();
            for(size_t p=0;p<pinned_.size();++p)
                for(size_t i=0;i<filtered.size();++i)
                    if(filtered[i]->executable==pinned_[p]) {visible_.push_back(filtered[i]);break;}
        } else std::stable_sort(visible_.begin(),visible_.end(),LibraryOrder(gameSort_));
    } else if (section_==Apps || section_==Emulators) {
        // Tabs are filters over the same installed XEX catalogue.
        // 0 Todos; 1 Instalados; 2 Plugins; 3 Homebrew (Apps only).
        for(size_t i=0;i<tools_.size() && i<toolKinds_.size();++i)
            if(subtab_==0 || toolKinds_[i]==subtab_) visible_.push_back(&tools_[i]);
    }
    clampRow(row_,Count());
}
void ConsoleHome::LoadScanPaths() {
    roots_=GameScanner::LoadRoots(dir_+"aurorax-scanpaths.txt",baseRoots_);
    roots_=GameScanner::LoadRoots(savedFile(dir_+"aurorax-scanpaths-ui.txt"),roots_);
    roots_=GameScanner::LoadRoots(dir_+"aurorax-aurora-paths.txt",roots_);
}
void ConsoleHome::ToggleScanPath() {
    if(folders_.size()<2 || !fileWorkspace_.CanBrowse(folders_.back())) {
        notice_="Abra uma pasta abaixo da raiz do dispositivo.";return;
    }
    const std::string path=GameScanner::ValidRoot(folders_.back());
    if(path.empty()) {notice_="Caminho invalido para varredura.";return;}
    const std::string config=dir_+"aurorax-scanpaths-ui.txt";
    std::vector<std::string> custom=GameScanner::LoadRoots(savedFile(config),std::vector<std::string>());
    size_t index=0;
    while(index<custom.size() && !GameScanner::SameRoot(custom[index],path))++index;
    const bool remove=index<custom.size();
    if(remove)custom.erase(custom.begin()+index);
    else {
        for(size_t i=0;i<roots_.size();++i)
            if(GameScanner::SameRoot(roots_[i],path)) {
                notice_="Pasta ja cadastrada em outro arquivo de configuracao.";return;
            }
        if(roots_.size()>=32 || custom.size()>=32) {
            notice_="Limite total de 32 pastas atingido.";return;
        }
        custom.push_back(path);
    }
    std::ofstream out((config+".tmp").c_str(),std::ios::trunc);
    for(size_t i=0;i<custom.size();++i)out<<custom[i]<<"\n";
    out.close();
    if(!out || !replaceSavedFile(config)) {
        notice_="Falha ao gravar caminhos.";return;
    }
    LoadScanPaths();
    notice_=remove?"Pasta removida da busca personalizada.":"Pasta adicionada. Biblioteca atualiza ao abrir.";
}
void ConsoleHome::Reload() {
    notice_.clear();
    artFolders_.clear();artDirty_=true;
    LoadScanPaths();
    if (section_==Files && page_!=Home) {
        if (!fileWorkspace_.CanBrowse(folders_.back()) || !ReadFolder(folders_.back(),files_)) {
            files_.clear();notice_="Dispositivo ou pasta indisponivel.";
        }
    } else if ((section_==Apps || section_==Emulators) && page_!=Home) {
        std::vector<std::string> roots;
        for(size_t i=0;i<roots_.size();++i) {
            size_t sep=roots_[i].find_last_of("\\/");
            roots.push_back(roots_[i].substr(0,sep+1)+(section_==Apps?"Apps":"Emulators"));
        }
        roots.push_back(FileWorkspace::Join(FileWorkspace::Join(dir_,"UserFiles"),section_==Apps?"Apps":"Emulators"));
        for(const char* id:{"hdd","usb0","usb1","usb2"}){
            const auto folder=StoreStorage::Personal(dir_,id);if(!folder.empty())roots.push_back(FileWorkspace::Join(folder,section_==Apps?"Apps":"Emulators"));
        }
        tools_=GameScanner(roots).Scan();
        toolKinds_.assign(tools_.size(),1);
        // Installed homebrew plugins are opt-in local XEX applications.
        const std::vector<PluginDescriptor> plugins=
            PluginRegistry(PluginRegistry::DefaultRoots(dir_)).Scan();
        const std::string category=section_==Apps?"app":"emulator";
        for(size_t i=0;i<plugins.size();++i) if(plugins[i].category==category&&PluginAllowed(plugins[i])) {
            Game app={plugins[i].name,plugins[i].executable,
                      "Plugin "+plugins[i].version,false};
            tools_.push_back(app);
            toolKinds_.push_back(2);
        }
        if(section_==Apps) {
            // Dedicated Homebrew tab: scan app-relative, HDD and USB folders.
            std::vector<std::string> homeRoots;
            const char separator=dir_.find('\\')!=std::string::npos?'\\':'/';
            homeRoots.push_back(dir_+(dir_.empty() || dir_[dir_.size()-1]==separator
                                      ?"":std::string(1,separator))+"Homebrew");
            for(size_t i=0;i<roots_.size();++i) {
                const size_t split=roots_[i].find_last_of("\\/");
                homeRoots.push_back(roots_[i].substr(0,split==std::string::npos?0:split+1)+"Homebrew");
            }
            homeRoots.push_back(FileWorkspace::Join(FileWorkspace::Join(dir_,"UserFiles"),"Homebrew"));
            for(const char* id:{"hdd","usb0","usb1","usb2"}){
                const auto folder=StoreStorage::Personal(dir_,id);if(!folder.empty())homeRoots.push_back(FileWorkspace::Join(folder,"Homebrew"));
            }
            const std::vector<Game> homebrew=GameScanner(homeRoots).Scan();
            for(size_t i=0;i<homebrew.size();++i) {
                bool duplicate=false;
                for(size_t j=0;j<tools_.size();++j)
                    if(tools_[j].executable==homebrew[i].executable) {duplicate=true;break;}
                if(!duplicate) {
                    Game app=homebrew[i];
                    app.device="Homebrew";
                    tools_.push_back(app);
                    toolKinds_.push_back(3);
                }
            }
        }
    } else ScanLibrary();
    Rebuild();
}
bool ConsoleHome::EnqueueCopy() {
    std::vector<std::string> names=copyNames_;
    if(names.empty()&&!copyName_.empty())names.push_back(copyName_);
    std::vector<WorkspaceCopyStep> steps;
    if(!fileWorkspace_.PlanCopies(copyDir_,names,folders_.back(),steps)) {
        notice_="Copia recusada: destino, conflito, nome ou limite de arvore.";return false;
    }
    if(copyQueue_.size()+steps.size()>256) {notice_="Fila cheia (256 operacoes).";return false;}
    for(size_t i=0;i<steps.size();++i) {
        const std::string target=FileWorkspace::Join(steps[i].destination,steps[i].name);
        if(!activeCopyTarget_.empty() && GameScanner::SameRoot(activeCopyTarget_,target)) {
            notice_="Destino ja em transferencia.";return false;
        }
        for(size_t j=0;j<copyQueue_.size();++j)
            if(GameScanner::SameRoot(FileWorkspace::Join(copyQueue_[j].destination,copyQueue_[j].name),target)) {
                notice_="Destino ja registrado na fila.";return false;
            }
    }
    const bool busy=TransferRunning();
    copyQueue_.insert(copyQueue_.end(),steps.begin(),steps.end());
    if(!busy) {
        copyPaused_=false;
        if(!StartNextCopy())return false;
    }
    notice_="Copia agendada: "+number(steps.size())+" operacoes. Sistema > Downloads.";
    RecordEvent("Copia agendada");return true;
}
bool ConsoleHome::StartNextCopy() {
    if(copyQueue_.empty())return false;
    const WorkspaceCopyStep task=copyQueue_.front();copyQueue_.erase(copyQueue_.begin());
    activeCopyTarget_.clear();
    if(task.directory) {
        // A newly appearing destination never becomes a merge target.
        if(!fileWorkspace_.CanBrowse(FileWorkspace::Join(task.source,task.name))||
           !fileWorkspace_.Create(task.destination,task.name)) {
            copyQueue_.clear();notice_="Pasta recusada; fila interrompida. Copias concluidas preservadas.";
            RecordEvent("Falha na criacao de pasta");return false;
        }
        notice_="Pasta criada para copia.";return true;
    }
    if(!transfer_.Start(fileWorkspace_,task.source,task.name,task.destination)) {
        copyQueue_.clear();notice_="Fila interrompida: "+transfer_.Error();RecordEvent("Falha ao iniciar copia");return false;
    }
    copyRate_.Reset();copyRate_.Sample(0,transfer_.Total(),TransferRate::Now());
    activeCopyTarget_=FileWorkspace::Join(task.destination,task.name);
    notice_="Proxima copia iniciada.";return true;
}
void ConsoleHome::LoadEvents() {
    eventFilter_=NotificationCenter::All;eventRow_=0;
    if(!notifications_.Load(dir_+"infinity360-events.log"))notice_="Historico invalido; arquivo preservado.";
}
void ConsoleHome::RecordEvent(const std::string& message) {
    // Fixed operational/session events only; no filenames, URLs or account data.
    const std::time_t now=std::time(0);
    std::tm* utc=std::gmtime(&now);char stamp[32]="hora indisponivel";
    if(now>=946684800&&utc)std::strftime(stamp,sizeof stamp,"%Y-%m-%d %H:%MZ",utc);
    if(!notifications_.Append(std::string(stamp)+" | "+message))return;
    if(!notifications_.Save(dir_+"infinity360-events.log"))notice_+=" (historico apenas nesta sessao)";
}
void ConsoleHome::MarkNotifications(bool all){
    const auto rows=notifications_.Visible(eventFilter_);
    auto previous=notifications_;
    if(all)notifications_.MarkAll();
    else if(eventRow_>=0&&static_cast<size_t>(eventRow_)<rows.size())notifications_.Mark(rows[eventRow_]);
    else return;
    if(!notifications_.Save(dir_+"infinity360-events.log")){
        notifications_=previous;notice_="Falha ao salvar leitura; historico preservado.";
    }else notice_=all?"Todas marcadas como lidas.":"Notificacao marcada como lida.";
    clampRow(eventRow_,notifications_.Visible(eventFilter_).size());
}
void ConsoleHome::MovePin(int direction) {
    const int neighbor=row_+direction;
    if(neighbor<0 || static_cast<size_t>(neighbor)>=visible_.size())return;
    const std::string a=visible_[row_]->executable,b=visible_[neighbor]->executable;
    const std::vector<std::string> old=pinned_;
    std::vector<std::string>::iterator first=std::find(pinned_.begin(),pinned_.end(),a);
    std::vector<std::string>::iterator second=std::find(pinned_.begin(),pinned_.end(),b);
    if(first==pinned_.end()||second==pinned_.end())return;
    std::iter_swap(first,second);
    if(!SaveProfile()) {
        pinned_=old;notice_="Falha ao salvar a ordem; alteracao cancelada.";return;
    }
    row_=neighbor;Rebuild();notice_="Ordem dos jogos fixados salva.";
}
bool ConsoleHome::Tick() {
    const bool peerChanged=peerProbe_.Tick();
    bool smbChanged=smbProbe_.Tick();
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMB_MENU)
    smbChanged=TickSmbSession()||smbChanged;
#endif
    bool socialChanged=false;
#if defined(__XSDK__)
    const uint64_t now=TransferRate::Now();
    if(!socialPolled_||now<lastSocialPoll_||now-lastSocialPoll_>=2000){
        socialPolled_=true;lastSocialPoll_=now;
        const auto before=social_;
        SetSocialSnapshot(InfinitySocial::ReadXsdkProfiles(!TransferRunning()&&InfinitySocial::GamerpicEnabled(dir_),before.controller),before.controller);
        socialChanged=!InfinitySocial::SameHeader(before,social_);
        if(const char* event=socialEvents_.Observe(social_)){RecordEvent(event);socialChanged=true;}
    }
#endif
    bool thermalChanged=false;
    if(ThermalTelemetry::Supported()&&!TransferRunning()){
        const auto stamp=TransferRate::Now();
        if(ThermalTelemetry::HeaderDue(headerThermalSampled_,headerThermalAt_,stamp)){
            headerThermalAt_=stamp;headerThermalSampled_=true;
            headerThermal_=ThermalTelemetry::Read();thermalChanged=true;
        }
    }
    const bool heroChanged=ApprovedCarouselTick(TransferRate::Now());
    const bool ftpChanged=ftp_.Tick()||peerChanged||smbChanged||socialChanged||thermalChanged||heroChanged;
    if(!TransferRunning() || copyPaused_)return ftpChanged;
    if(transfer_.Status()!=FileTransfer::Running) {
        StartNextCopy();
        if(!TransferRunning() && section_==Files && page_==Listing && !searchActive_)Reload();
        return true;
    }
    const unsigned previous=transfer_.Percent();
    transfer_.Tick();
    const bool rateChanged=copyRate_.Sample(transfer_.Done(),transfer_.Total(),TransferRate::Now());
    if(transfer_.Status()!=FileTransfer::Running) {
        activeCopyTarget_.clear();
        if(transfer_.Status()!=FileTransfer::Completed)copyQueue_.clear();
        if(section_==Files && page_==Listing && !searchActive_)Reload();
        notice_=transfer_.Status()==FileTransfer::Completed?"Arquivo copiado.":
            "Copia interrompida: "+transfer_.Error();
        RecordEvent(transfer_.Status()==FileTransfer::Completed?"Arquivo copiado":"Copia interrompida");return true;
    }
    return ftpChanged || rateChanged || previous!=transfer_.Percent();
}
std::string ConsoleHome::TakeMovie() {
    const std::string movie=tv_.TakeMovie();
    if(!movie.empty() && TransferRunning()) {
        tv_.PlayerMessage("Copia em andamento. Aguarde ou cancele em Sistema > Downloads.");
        return "";
    }
    return movie;
}
void ConsoleHome::RefreshStorage() {
    storageVolumes_=StorageInfo::ReadAll(fileWorkspace_.Roots());
    if(storagePage_*4>=storageVolumes_.size())storagePage_=0;
}
void ConsoleHome::RefreshResidentPlugins() {
    const std::string previous=residentSource_<residentConfigs_.size()?residentConfigs_[residentSource_].path:"";
    // Inventory every accessible candidate; no claim about active boot source.
    std::vector<std::string> paths;
    paths.push_back(dir_+"launch.ini");
#ifdef __XSDK__
    paths.push_back("hdd:/launch.ini");paths.push_back("usb0:/launch.ini");paths.push_back("usb1:/launch.ini");paths.push_back("usb2:/launch.ini");
#elif defined(_XBOX)
    paths.push_back("HDD1:\\launch.ini");
    paths.push_back("USB0:\\launch.ini");
    paths.push_back("USB1:\\launch.ini");
#endif
    residentConfigs_=residentInventory(paths);residentSource_=0;
    for(size_t i=0;i<residentConfigs_.size();++i)if(residentConfigs_[i].path==previous){residentSource_=i;break;}
}
void ConsoleHome::ToggleFtp(bool allowUpload) {
    if(ftp_.Running()){ftp_.Stop();notice_="FTP encerrado.";RecordEvent("FTP encerrado");return;}
#if !defined(_XBOX) && !defined(__XSDK__)
    (void)allowUpload;
    notice_="FTP nativo: somente no Xbox; teste de transporte separado no PC.";
#else
#ifdef __XSDK__
    if(allowUpload){
        std::ifstream upload((dir_+"enable-ftp-upload.txt").c_str());
        std::string enabled;
        if(!std::getline(upload,enabled)||enabled!="ENABLE-FTP-UPLOAD"){
            notice_="Upload bloqueado: habilite enable-ftp-upload.txt.";return;
        }
    }
    std::ifstream opt((dir_+"enable-ftp.txt").c_str());
    std::string signature;
    if(!std::getline(opt,signature)||signature!="ENABLE-FTP-LAN"){
        notice_="FTP bloqueado: habilite enable-ftp.txt na pasta de teste.";return;
    }
#endif
    RefreshNetwork();
    std::ifstream in((dir_+"infinity360-ftp.txt").c_str(),std::ios::binary);
    char data[132];
    in.read(data,sizeof data);
    if(in.bad()||in.gcount()>=static_cast<std::streamsize>(sizeof data)) {
        notice_="Arquivo de credenciais FTP invalido.";return;
    }
    std::istringstream lines(std::string(data,static_cast<size_t>(in.gcount())));
    std::string user,password,extra;
    if(!std::getline(lines,user)||!std::getline(lines,password)||std::getline(lines,extra)) {
        notice_="Configure infinity360-ftp.txt: usuario e senha em duas linhas.";return;
    }
    if(!user.empty()&&user[user.size()-1]=='\r')user.erase(user.size()-1);
    if(!password.empty()&&password[password.size()-1]=='\r')password.erase(password.size()-1);
    const bool started=ftp_.Start(FileWorkspace::Join(dir_,"UserFiles"),network_.ipv4,2121,user,password,allowUpload);
    notice_=started?(allowUpload?"FTP LAN leitura/upload (ALLO) iniciado.":"FTP LAN somente leitura iniciado."):ftp_.Error();
    RecordEvent(started?"FTP iniciado":"Falha ao iniciar FTP");
#endif
}
void ConsoleHome::RefreshNetwork() {
    network_=NetworkInfo::Read();
}
void ConsoleHome::Enter(Section s) {
    section_=s; page_=Listing; row_=0; subtab_=0; systemTab_=0;gridFilter_=false;
    collectionPicker_=false; searchActive_=false; keyboardMode_=0;
    if(s==Iptv) {tv_.Open();return;}
    if(s==System) RefreshStorage();
    if (s==Files) {folders_.clear();fileRoot_=0;pasteConfirm_=false;scanPathConfirm_=false;fileAction_=false;deleteConfirm_=false;folders_.push_back(fileWorkspace_.Roots()[0]);}
    Reload();
}
void ConsoleHome::StartFileEdit(bool rename) {
    if(rename && (!Count() || !fileWorkspace_.CanWrite(folders_.back()))) {
        notice_="Renomear: selecione um item em UserFiles/AuroraXFiles.";
        return;
    }
    renameName_=rename?files_[row_].name:"";
    searchDraft_=renameName_;
    if(!rename && !fileWorkspace_.CanWrite(folders_.back())) {
        const std::string target=fileWorkspace_.WorkspaceFor(folders_.back());
        if(target.empty()) {notice_="Crie pastas somente no espaco de arquivos pessoais.";return;}
        searchDraft_=target.substr(target.find_last_of("\\/")+1);
    }
    keyboardMode_=rename?4:3;searchCursor_=0;searchActive_=true;notice_.clear();
}
void ConsoleHome::ConfirmFileEdit() {
    const bool renamed=keyboardMode_==4;
    const bool success=renamed?
        fileWorkspace_.Rename(folders_.back(),renameName_,searchDraft_):
        fileWorkspace_.Create(folders_.back(),searchDraft_);
    if(!success) {
        notice_="Operacao recusada: nome, destino, acesso ou arquivo existente.";
        return;
    }
    searchActive_=false;keyboardMode_=0;
    Reload();
    notice_=renamed?"Item renomeado.":"Pasta criada.";
}
void ConsoleHome::ApplySearch() {
    if(keyboardMode_==3 || keyboardMode_==4) {ConfirmFileEdit();return;}
    if(keyboardMode_==1) {
        // New collection; optionally assign the selected game immediately.
        if(!collections_.Create(searchDraft_)) {
            notice_="Nome invalido, repetido ou limite de colecoes atingido.";
            return;
        }
        collectionIndex_=static_cast<int>(collections_.Items().size()-1);
        if(!pickerGame_.empty())
            collections_.Toggle(static_cast<size_t>(collectionIndex_),pickerGame_);
        const bool persisted=collections_.Save();
        searchActive_=false;
        collectionPicker_=false;
        keyboardMode_=0;
        subtab_=5;
        row_=0;
        Rebuild();
        notice_=persisted?"Colecao criada.":"Colecao criada, mas nao foi possivel salvar.";
        return;
    }
    if(keyboardMode_==2) {
        if(collectionIndex_<0 || !collections_.Rename(
            static_cast<size_t>(collectionIndex_),searchDraft_)) {
            notice_="Nome invalido ou ja utilizado por outra colecao.";
            return;
        }
        const bool persisted=collections_.Save();
        searchActive_=false;
        collectionPicker_=false;
        keyboardMode_=0;
        subtab_=5;
        row_=0;
        Rebuild();
        notice_=persisted?"Colecao renomeada.":"Nome alterado apenas nesta sessao.";
        return;
    }
    searchTerm_=searchDraft_;
    searchActive_=false;
    row_=0;
    Rebuild();
    notice_=searchTerm_.empty()?"Busca removida":"Busca: "+searchTerm_+
            " ("+number(Count())+" jogos)";
}
std::string ConsoleHome::ArtFolder(const Game& game) const {
    const std::map<std::string,std::string>::const_iterator found=artFolders_.find(game.executable);
    if(found!=artFolders_.end())return found->second;
    const XboxTitleMetadata info=ContentCatalog::DescribeTitle(game.executable,isContentPackage(game));
    std::string folder;
    if(info.known && info.titleId) {
        char id[9];std::sprintf(id,"%08X",info.titleId);
        folder=dir_+"Art/"+id+"/";
        // FATX/XDK paths use backslashes.
        if(dir_.find('\\')!=std::string::npos)
            std::replace(folder.begin(),folder.end(),'/','\\');
    }
    if(artFolders_.size()>=128)artFolders_.clear();
    artFolders_[game.executable]=folder;
    return folder;
}
std::string ConsoleHome::ArtPath(const Game& game,bool cover) const {
    const std::string folder=ArtFolder(game);
    if(folder.empty())return "";
    const std::string id=folder.substr(folder.size()-9,8);
    const std::map<std::string,std::string>::const_iterator it=artChoices_.find(id);
    const char choice=it==artChoices_.end()?'0':it->second[cover?0:1];
    const auto local=artFile(folder,cover,choice);
    if(choice!='0'||local.empty()||fileExists(local))return local;
    const std::string idPath=FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(dir_,"UserFiles"),"Art"),id);
    return FileWorkspace::Join(idPath,cover?"cover.rgba":"background.rgba");
}
std::string ConsoleHome::ReverseArtPath(const Game& game)const{
    const auto folder=ArtFolder(game);
    if(folder.empty())return "";
    const auto legacy=folder+"cover-back.rgba";
    if(fileExists(legacy))return legacy;
    const auto id=folder.substr(folder.size()-9,8);
    return FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(dir_,"UserFiles"),"Art"),id),"cover-back.rgba");
}
void ConsoleHome::RefreshTitleMetadata() {
    detailMetadata_=XboxTitleMetadata();
    if(!Count() || section_==Files || section_==System)return;
    const Game& game=*visible_[row_];
    detailMetadata_=ContentCatalog::DescribeTitle(game.executable,isContentPackage(game));
}

void ConsoleHome::OpenContent() {
    contentActive_=true;contentInstallActive_=false;contentInstallConfirm_=false;contentInstallPending_=false;contentInstallStaged_.clear();contentInstallBackup_.clear();
    unityActive_=false;unityPending_=false;unityDownloadPending_=false;unityConfirm_=false;unityUpdates_.clear();unityRow_=0;
    contentKnown_=false;
    contentInfo_=XboxExecutionInfo();
    contentItems_.clear();stagedItems_.clear();archivedItems_.clear();disabledItems_.clear();toggleTuConfirm_=false;restoreArchivedConfirm_=false;backups_.clear();trainers_.clear();trainerConfirm_=false;backupConfirm_=false;backupPending_=false;recoverConfirm_=false;recoverPending_=false;verifyOnly_=false;archiveTuConfirm_=false;
    contentTab_=0;
    contentRow_=0;
    notice_.clear();
    RefreshTitleMetadata();
    if(!detailMetadata_.known)return;
    contentKnown_=true;
    contentInfo_.titleId=detailMetadata_.titleId;
    contentInfo_.mediaId=detailMetadata_.mediaId;
    contentInfo_.version=detailMetadata_.version;
    contentInfo_.baseVersion=detailMetadata_.baseVersion;
    contentInfo_.discNumber=detailMetadata_.discNumber;
    contentInfo_.discCount=detailMetadata_.discCount;
    contentItems_=ContentCatalog(ContentCatalog::DefaultRoots(dir_)).ScanExtras(contentInfo_.titleId);
    trainers_=TrainerCatalog::Scan(dir_,contentInfo_.titleId);
    stagedItems_=ContentBackup::Staged(fileWorkspace_,dir_,contentInfo_.titleId);
    backups_=ContentBackup::List(fileWorkspace_,dir_,contentInfo_.titleId);
    archivedItems_=ContentBackup::Archived(fileWorkspace_,dir_,contentInfo_.titleId);
    disabledItems_=ContentInstall::DisabledUpdates(fileWorkspace_,dir_,contentInfo_.titleId);
}

size_t ConsoleHome::ContentCount() const {
    if(contentTab_==3)return trainers_.size();
    if(contentTab_==4)return stagedItems_.size();
    if(contentTab_==5)return backups_.size();
    if(contentTab_==6)return archivedItems_.size();
    if(contentTab_==7)return disabledItems_.size();
    const unsigned types[]={0x000b0000u,2u,1u};
    size_t count=0;
    for(size_t i=0;i<contentItems_.size();++i)
        if(contentItems_[i].contentType==types[contentTab_])++count;
    return count;
}
const XboxContentPackage* ConsoleHome::ContentAt(size_t index) const {
    if(contentTab_==3)return 0;
    if(contentTab_==4)return index<stagedItems_.size()?&stagedItems_[index]:nullptr;
    if(contentTab_==5)return index<backups_.size()?&backups_[index].package:nullptr;
    if(contentTab_==6)return index<archivedItems_.size()?&archivedItems_[index]:nullptr;
    if(contentTab_==7)return index<disabledItems_.size()?&disabledItems_[index]:nullptr;
    const unsigned types[]={0x000b0000u,2u,1u};
    for(size_t i=0;i<contentItems_.size();++i) {
        if(contentItems_[i].contentType!=types[contentTab_])continue;
        if(index==0)return &contentItems_[i];
        --index;
    }
    return 0;
}
void ConsoleHome::RecordLaunchAttempt(const std::string& path){
    if(!Installed(path))return;
    const auto now=std::time(nullptr);launchStats_.Attempt(path,now<0?0:static_cast<uint64_t>(now));
    if(launched_.size()<4096&&!contains(launched_,path))launched_.push_back(path);
    history_.erase(std::remove(history_.begin(),history_.end(),path),history_.end());
    history_.insert(history_.begin(),path);
    if(history_.size()>HistoryLimit)history_.resize(HistoryLimit);
    if(!SaveProfile())notice_="Historico desta sessao; falha ao salvar.";
}
void ConsoleHome::RecordLaunchFailure(const std::string& path){
    if(launchStats_.Failed(path)){if(!SaveProfile())notice_="Falha ao salvar estatistica de lancamento.";RecordEvent("Falha de lancamento");}
}
void ConsoleHome::LaunchFailed() { notice_="O console não iniciou o título. Confira se o dispositivo está conectado."; RecordEvent("Falha de lancamento"); }

void ConsoleHome::PreviewAuroraProfile() {
    importFavorites_.clear();importHistory_.clear();
    std::ifstream in((dir_+"aurorax-aurora-profile.tsv").c_str(),std::ios::binary);
    if(!in) {notice_="Exportacao de perfil Aurora nao encontrada.";return;}
    const size_t maxBytes=128*1024;
    std::vector<char> data(maxBytes+1);
    in.read(&data[0],static_cast<std::streamsize>(data.size()));
    if(in.bad() || in.gcount()>static_cast<std::streamsize>(maxBytes)) {
        notice_="Exportacao de perfil invalida ou grande demais.";return;
    }
    // Only unique locally identified titles. Ambiguous multi-disc or duplicate
    // installations require a later path-aware import rather than guessing.
    std::map<std::string,std::string> local;
    for(size_t i=0;i<games_.size();++i) {
        const Game& game=games_[i];
        const std::string folder=ArtFolder(game);
        if(folder.empty())continue;
        const std::string id=folder.substr(folder.size()-9,8);
        if(local.count(id)) {
            if(local[id]!=game.executable)local[id].clear();
        } else local[id]=game.executable;
    }
    std::istringstream lines(std::string(&data[0],static_cast<size_t>(in.gcount())));
    std::string line;
    while(std::getline(lines,line)) {
        if(!line.empty() && line[line.size()-1]=='\r')line.erase(line.size()-1);
        if(line.size()!=10 || line[1]!='\t' || !validArtId(line.substr(2)))continue;
        const std::map<std::string,std::string>::const_iterator it=local.find(line.substr(2));
        if(it==local.end() || it->second.empty())continue;
        if(line[0]=='F' && !contains(favorites_,it->second) &&
           !contains(importFavorites_,it->second) && importFavorites_.size()<4096)
            importFavorites_.push_back(it->second);
        // Only executable games may be imported into the local play ledger.
        const Game* game=Installed(it->second);
        if(line[0]=='R' && game && isXexPath(game->executable) && !isContentPackage(*game) &&
           !contains(importHistory_,it->second) && importHistory_.size()<4096)
            importHistory_.push_back(it->second);
    }
    importPreview_=true;notice_.clear();
}
void ConsoleHome::ApplyAuroraProfile() {
    const std::vector<std::string> oldFavorites=favorites_, oldHistory=history_, oldLaunched=launched_;
    for(size_t i=0;i<importFavorites_.size();++i)
        if(!contains(favorites_,importFavorites_[i]))favorites_.push_back(importFavorites_[i]);
    for(size_t i=0;i<importHistory_.size();++i) {
        const std::string& path=importHistory_[i];
        if(history_.size()<HistoryLimit && !contains(history_,path))history_.push_back(path);
        if(launched_.size()<4096 && !contains(launched_,path))launched_.push_back(path);
    }
    if(!SaveProfile()) {
        favorites_=oldFavorites;history_=oldHistory;launched_=oldLaunched;
        notice_="Falha ao salvar. A tenta novamente; B cancela.";return;
    }
    importPreview_=false;notice_="Perfil Aurora mesclado; preferencias locais preservadas.";
}
void ConsoleHome::RebuildStore() {
    static const char* const tabs[]={"","game","xbla","homebrew","app","emulator","dlc","translation"};
    storeVisible_.clear();
    for(size_t i=0;i<store_.Items().size();++i)
        if((storeTab_==0||store_.Items()[i].category==tabs[storeTab_])&&StoreSourceDirectory::Matches(storeSource_,store_.Items()[i]))storeVisible_.push_back(i);
    clampRow(storeRow_,storeVisible_.size());
}
bool ConsoleHome::StartNativeStore(const StoreItem& item){
#if defined(INFINITY_XSDK_HTTPS) || defined(_XBOX)
    if(TransferRunning()||!TranslationMatches(item)){notice_="Copia ativa ou traducao incompativel.";return false;}
    if(store_.Staged(item)){notice_="Arquivo local ja existe; RB gerencia pacote na ficha.";return false;}
    if(store_.PackagePresent(item)){notice_="Pacote anterior presente; RB gerenciar > arquivar pacote.";return false;}
    nativeStoreItem_=item;nativeStorePending_=true;notice_="Preparando download HTTPS nativo.";return true;
#else
    (void)item;notice_="Provider HTTPS indisponivel.";return false;
#endif
}
bool ConsoleHome::NextNativeStore(){
    for(size_t index:store_.NativeQueue()){
        const StoreItem item=store_.Items()[index];
        const StoreProgress::State state=store_.Progress(item).state;
        if(state==StoreProgress::NativeVerified||state==StoreProgress::Verified){
            if(!store_.RemoveNative(item)){notice_="Falha ao salvar fila nativa.";return false;}
            continue;
        }
        return StartNativeStore(item);
    }
    notice_=!store_.NativeQueueValid()?"Fila invalida; arquivo preservado.":store_.OrphanNativeCount()?"Fila possui referencias ausentes; LB revisa limpeza.":"Fila nativa concluida.";return false;
}
bool ConsoleHome::TakeStoreInstallRequest(StoreItem& item){
    if(!storeInstallPending_)return false;
    storeInstallPending_=false;item=storeInstallItem_;return true;
}
void ConsoleHome::StoreInstallResult(const std::string& message,bool success){
    if(success)Reload();notice_=message;RecordEvent(success?"Pacote autorizado instalado":"Instalacao interrompida");
}
bool ConsoleHome::TakeProviderRequest(StoreProvider& provider){
    if(!providerPending_)return false;
    providerPending_=false;provider=providerRequest_;return true;
}
void ConsoleHome::ProviderResult(const std::string& message,bool success){
    store_.Load();storeRow_=0;RebuildStore();clampRow(providerRow_,StoreSourceDirectory::Rows(store_.Providers()).size());
    notice_=message;RecordEvent(success?"Catalogo remoto sincronizado":"Sincronizacao recusada");
}
bool ConsoleHome::TakeNativeStoreRequest(StoreItem& item) {
    if(!nativeStorePending_)return false;
    nativeStorePending_=false;item=nativeStoreItem_;return true;
}
void ConsoleHome::NativeStoreResult(const std::string& message,bool success) {
    notice_=message;RecordEvent(success?"Download nativo verificado":"Download nativo interrompido");
    if(success&&store_.NativeQueued(nativeStoreItem_)&&!store_.RemoveNative(nativeStoreItem_)){
        nativeBatch_=false;notice_+=" | Falha ao salvar fila.";
    }
    if(nativeBatch_){
        if(!success||!NextNativeStore())nativeBatch_=false;
    }
    if(storeDownloads_)RebuildStoreDownloads();
}
bool ConsoleHome::TranslationMatches(const StoreItem& item) const {
    if(item.category!="translation")return true;
    for(size_t i=0;i<games_.size();++i) {
        if(isContentPackage(games_[i]))continue;
        XboxExecutionInfo info;
        if(ContentCatalog::ReadExecutionInfo(games_[i].executable,info) &&
           info.titleId==item.titleId&&info.mediaId==item.mediaId&&info.version==item.gameVersion)
            return true;
    }
    return false;
}
void ConsoleHome::OpenQuickGuideAction(unsigned action){
    quickGuideOpen_=false;quickGuideEditing_=false;notice_.clear();
    switch(action){
        case QuickGuide::Games:Enter(Library);break;
        case QuickGuide::Recent:Enter(Recent);break;
        case QuickGuide::Pinned:Enter(Favorites);break;
        case QuickGuide::TV:Enter(Iptv);break;
        case QuickGuide::Notifications:
            section_=System;systemTab_=5;page_=Details;eventFilter_=NotificationCenter::All;eventRow_=0;break;
        case QuickGuide::Transfers:
            section_=System;systemTab_=3;page_=Details;storeDownloads_=false;storeDownloadRow_=0;break;
        case QuickGuide::Settings:
            section_=System;systemTab_=7;page_=Details;localMilestones_=false;saveCenter_=false;achievementsPage_=false;appearanceEditor_=false;powerPage_=false;powerConfirm_.Cancel();status_=ConsoleStatus::Read();break;
        case QuickGuide::Store:
            storeActive_=true;storeDetails_=false;storeConfirm_=false;storeRow_=0;storeTab_=0;
            notice_=store_.Load()?"":"Catalogo ausente: configure aurorax-store.tsv.";RebuildStore();break;
        default:break;
    }
}
std::string ConsoleHome::Press(Key key) {
    if(approvedGroup_>=0){
        const auto actions=ApprovedHome::GroupActions(approvedGroup_,approvedShortcuts_);
        const int count=actions[3]<0?3:4;
        if(key==Back){approvedGroup_=-1;notice_.clear();return "";}
        if(key==Up||key==Down)approvedGroupRow_=std::max(0,std::min(count-1,approvedGroupRow_+(key==Up?-1:1)));
        if(key==Accept){const int action=actions[approvedGroupRow_];approvedGroup_=-1;OpenApprovedAction(action);}
        return "";
    }
    if(weatherPanel_){if(key==Back){weatherPanel_=false;notice_.clear();}else if((key==Refresh||key==Accept)&&clockSettings_.city)weatherPending_=true;return "";}
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMB_MENU)
    if(smbUi_!=SmbClosed){PressSmbSession(key);return "";}
#endif
    if(quickGuideOpen_){
        if(!quickGuideEditing_){
            if(key==Right){socialGuideFocus_=true;notice_.clear();return "";}
            if(key==Left&&socialGuideFocus_){socialGuideFocus_=false;notice_.clear();return "";}
            if(socialGuideFocus_){
                if(key==Up||key==Down){socialGuideRow_=1-socialGuideRow_;notice_.clear();return "";}
                if(key==Accept){
                    const auto action=socialGuideRow_==0?InfinitySocial::NativeAction::GamerCard:InfinitySocial::NativeAction::Friends;
                    if(TransferRunning()){notice_="Conclua ou cancele as copias primeiro.";return "";}
#if defined(__XSDK__)
                    if(InfinitySocial::CanRequestNative(action,social_))nativeSocialPending_=action;
                    else notice_=action==InfinitySocial::NativeAction::Friends?"Amigos requerem perfil online do console.":"Cartao requer perfil ativo do console.";
#else
                    (void)action;notice_="Interface nativa experimental apenas no Xbox 360.";
#endif
                    return "";
                }
                if(key==Favorite){socialGuideFocus_=false;notice_.clear();return "";}
            }
        }
        if(key==Back){if(quickGuideEditing_){quickGuideEditing_=false;quickGuideDraft_.clear();quickGuideRow_=quickGuideEditStartRow_;}else quickGuideOpen_=false;notice_.clear();return "";}
        if(key==Up&&quickGuideRow_>0)--quickGuideRow_;
        if(key==Down&&quickGuideRow_<QuickGuide::Count-1)++quickGuideRow_;
        if(quickGuideEditing_){
            if(key==PageUp||key==PageDown)QuickGuide::Move(quickGuideDraft_,quickGuideRow_,key==PageUp?-1:1);
            if(key==Favorite){quickGuideDraft_=QuickGuide::Default();quickGuideRow_=0;}
            if(key==Refresh){
                const auto old=quickGuideOrder_;quickGuideOrder_=quickGuideDraft_;
                if(!SaveProfile()){quickGuideOrder_=old;notice_="Falha ao salvar guia.";}
                else{quickGuideEditing_=false;notice_="Guia personalizado salvo.";}
            }
        }else{
            if(key==PageUp){
                if(TransferRunning()){notice_="Conclua ou cancele as copias antes de trocar perfil.";return "";}
#if defined(__XSDK__)
                signinPending_=true;
#else
                notice_="Selecao oficial de perfil: disponivel somente no XSDK.";
#endif
                return "";
            }
            if(key==PageDown){
                if(TransferRunning()){notice_="Conclua ou cancele as copias antes de abrir conquistas.";return "";}
                quickGuideOpen_=false;quickGuideEditing_=false;
                OpenApprovedAction(ApprovedHome::Achievements);
                return "";
            }
            if(key==Search){quickGuideOpen_=false;notice_.clear();}
            if(key==Favorite){quickGuideDraft_=quickGuideOrder_;quickGuideEditStartRow_=quickGuideRow_;quickGuideEditing_=true;notice_.clear();}
            if(key==Accept)OpenQuickGuideAction(static_cast<unsigned>(quickGuideOrder_[quickGuideRow_]-'0'));
        }
        return "";
    }
    if(updateActive_){
        if(key==Back){if(updateConfirm_)updateConfirm_=false;else updateActive_=false;notice_.clear();return "";}
        if(key==Refresh){RefreshDashboardUpdate();return "";}
#if defined(INFINITY_STORE_ZIP)
        if(key==Accept){
            if(TransferRunning()||!updateKnown_||!DashboardUpdate::Enabled(dir_)){notice_="Manifesto, permissao ou transferencia ativa.";return "";}
            if(!updateConfirm_){updateConfirm_=true;notice_="A confirma instalar em pasta separada; versao atual intacta.";return "";}
            updateConfirm_=false;updatePending_=true;
        }
#endif
        return "";
    }
    if(discSetActive_){
        if(key==Back){discSetActive_=false;notice_.clear();return "";}
        if(key==Up&&discSetRow_>0)--discSetRow_;
        if(key==Down&&discSetRow_+1<discSet_.size())++discSetRow_;
        if(key==Refresh){discSet_=DiscSet::Find(games_,discSetIdentity_);discSetRow_=0;return "";}
        if(key==Accept&&discSetRow_<discSet_.size()){
            if(TransferRunning()){notice_="Conclua ou cancele as copias.";return "";}
            const auto& selected=discSet_[discSetRow_];XboxExecutionInfo info{};
            if(!DiscSet::ReadIdentity(selected.path,info)||!DiscSet::Matches(discSetIdentity_,info)||
               info.discNumber!=selected.info.discNumber||info.mediaId!=selected.info.mediaId||info.version!=selected.info.version){
                notice_="Disco mudou ou indisponivel. Y atualiza.";return "";
            }
            return selected.path;
        }
        return "";
    }
    if(coverBatchConfirm_&&key!=Guide)coverBatchConfirm_=false;
    if(section_==System&&page_==Details&&systemTab_==6&&!storeActive_){
        if(key==Refresh){RefreshDiscInstall();notice_="DVD e destino consultados.";return "";}
        if(key==Back&&discConfirm_){discConfirm_=false;notice_="Confirmacao cancelada.";return "";}
    }
    if(storeActive_) {
        if(storeProviders_){
            if(key==Back){storeProviders_=false;notice_.clear();return "";}
            if(key==Refresh){store_.Load();providerRow_=0;RebuildStore();notice_="Configuracao e caches recarregados.";return "";}
            const auto& providers=store_.Providers();
            const auto rows=StoreSourceDirectory::Rows(providers);
            clampRow(providerRow_,rows.size());
            if(key==Up&&providerRow_>0)--providerRow_;
            if(key==Down&&static_cast<size_t>(providerRow_+1)<rows.size())++providerRow_;
            const auto& selected=rows[providerRow_];
            if(key==Favorite){
                storeSource_="*";storeRow_=0;RebuildStore();storeProviders_=false;notice_.clear();return "";
            }
            if(key==Accept){
                if(!selected.filter){notice_="XboxUnity: abra detalhes de um jogo para capas ou TU.";return "";}
                storeSource_=selected.id;storeTab_=0;storeRow_=0;RebuildStore();storeProviders_=false;
                notice_=storeVisible_.empty()?"Fonte sem itens aprovados. Configure um catalogo TSV.":"";return "";
            }
            if(key==Guide){
                if(selected.provider<0){notice_="Fonte sem endpoint TSV aprovado; consulte FONTES.txt.";return "";}
#if defined(INFINITY_XSDK_HTTPS)
                if(TransferRunning()){notice_="Conclua a transferencia ativa.";return "";}
                providerRequest_=providers[selected.provider];providerPending_=true;notice_="Sincronizando catalogo autorizado.";
#else
                notice_="Sincronizacao nativa indisponivel.";
#endif
            }
            return "";
        }
        if(storeManage_){
            if(key==Back){if(storeManageConfirm_)storeManageConfirm_=false;else storeManage_=false;notice_.clear();return "";}
            if(key==Up||key==Down){storeManageRow_=(storeManageRow_+(key==Down?1:2))%3;storeManageConfirm_=false;return "";}
#if defined(INFINITY_XSDK_HTTPS)
            if(key==Accept&&storeDetails_&&!storeVisible_.empty()){
                if(TransferRunning()){notice_="Conclua a transferencia ativa.";return "";}
                if(!storeManageConfirm_){storeManageConfirm_=true;notice_="A novamente confirma; arquivos serao preservados.";return "";}
                storeManageConfirm_=false;StopFtp();
                const StoreItem item=store_.Items()[storeVisible_[storeRow_]];
                std::string detail;bool ok=false;
                if(storeManageRow_==0){ok=store_.ArchiveInstalled(item,detail);notice_=ok?"Aplicativo removido da biblioteca; backup preservado.":"Backup recusado; pasta indisponivel ou limite atingido.";}
                else if(storeManageRow_==1){ok=store_.RestoreInstalled(item,detail);notice_=ok?"Backup restaurado; versao anterior preservada.":detail;}
                else{ok=store_.ArchivePackage(item,detail);notice_=ok?"Pacote anterior arquivado. START baixa novo pacote.":"Pacote indisponivel ou limite de backups.";}
                if(ok){const auto status=notice_;Reload();notice_=status;}
                RecordEvent(ok?"Gerenciamento Store concluido":"Gerenciamento Store recusado");
            }
#endif
            return "";
        }
        if(key!=Search&&notice_=="R3 novamente: confirmar instalacao?")notice_.clear();
        if(key==Back) {
            if(storeConfirm_)storeConfirm_=false;
            else if(storeDetails_)storeDetails_=false;
            else storeActive_=false;
            notice_.clear();return "";
        }
        if(key==Refresh) {
            storeConfirm_=false;storeDetails_=false;storeRow_=0;
            notice_=store_.Load()?"Catalogo recarregado.":"Catalogo ausente ou invalido.";
            RebuildStore();return "";
        }
        if(!storeDetails_) {
            if(key==PageUp){
                if(TransferRunning()||nativeStorePending_||nativeBatch_){notice_="Conclua a transferencia ativa.";return "";}
                const std::vector<std::string> ids={"","hdd","usb0","usb1","usb2"};
                const auto it=std::find(ids.begin(),ids.end(),store_.Storage());
                const size_t start=it==ids.end()?0:static_cast<size_t>(it-ids.begin()+1);
                for(size_t offset=0;offset<ids.size();++offset){
                    const auto& id=ids[(start+offset)%ids.size()];
                    const auto folder=StoreStorage::Personal(dir_,id);
                    if(id==store_.Storage()||folder.empty()||!fileWorkspace_.CanWrite(folder))continue;
                    if(store_.SelectStorage(id)){storeRow_=0;RebuildStore();notice_="Destino Store: "+StoreStorage::Label(id)+"; fila independente.";return "";}
                }
                notice_="Nenhum outro workspace gravavel. Prepare Infinity360Files no dispositivo.";return "";
            }
            if(key==Search){storeProviders_=true;providerRow_=0;notice_.clear();return "";}
            if(key==Left&&storeTab_>0) {--storeTab_;storeRow_=0;RebuildStore();}
            if(key==Right&&storeTab_<7) {++storeTab_;storeRow_=0;RebuildStore();}
            if(key==Up&&storeRow_>0)--storeRow_;
            if(key==Down&&static_cast<size_t>(storeRow_+1)<storeVisible_.size())++storeRow_;
            if(key==Accept&&!storeVisible_.empty()) {
                storeDetails_=true;
                storeTranslationMatch_=TranslationMatches(store_.Items()[storeVisible_[storeRow_]]);
            }
        } else if(!storeVisible_.empty()) {
            const StoreItem& item=store_.Items()[storeVisible_[storeRow_]];
            if(key==Guide) {
#if defined(_XBOX) || defined(INFINITY_XSDK_HTTPS)
                nativeBatch_=false;storeConfirm_=false;StartNativeStore(item);
#else
                notice_="Download nativo disponivel no Xbox; testes usam o gravador comum.";
#endif
                return "";
            }
            if(key==Search){
#if defined(INFINITY_XSDK_HTTPS)
                if(TransferRunning()||!store_.Staged(item)||store_.Installed(item)||(item.category!="app"&&item.category!="homebrew"&&item.category!="emulator")){
                    notice_="Instalacao: pacote autorizado, baixado e ainda nao instalado.";return "";
                }
                if(notice_!="R3 novamente: confirmar instalacao?"){notice_="R3 novamente: confirmar instalacao?";return "";}
                storeInstallItem_=item;storeInstallPending_=true;notice_="Verificando e instalando XEX.";
#else
                notice_="Instalador nativo indisponivel.";
#endif
                return "";
            }
            if(key==PageUp){
#if defined(INFINITY_XSDK_HTTPS)
                if(store_.NativeQueued(item))notice_=store_.RemoveNative(item)?"Removido da fila nativa; parcial preservado.":"Falha ao salvar fila.";
                else if(store_.Staged(item)||!TranslationMatches(item))notice_="Item local ou traducao incompativel.";
                else notice_=store_.QueueNative(item)?"Adicionado a fila nativa.":"Fila cheia, invalida ou falha ao salvar.";
#else
                notice_="Fila nativa disponivel no XSDK HTTPS.";
#endif
                storeConfirm_=false;return "";
            }
            if(key==PageDown && store_.Progress(item).state!=StoreProgress::NativePaused&&(item.category=="app"||item.category=="homebrew"||item.category=="emulator")){
                storeManage_=true;storeManageConfirm_=false;storeManageRow_=0;storeConfirm_=false;notice_.clear();return "";
            }
            if(key==PageDown && store_.Progress(item).state==StoreProgress::NativePaused){
                // Explicit two-press discard, separate from removing a PC request.
                if(notice_!="RB novamente: descartar parcial?"){notice_="RB novamente: descartar parcial?";return "";}
                notice_=store_.DiscardPartial(item)?"Parcial descartado.":"Nao foi possivel descartar parcial.";
                return "";
            }
            if(key==Favorite) {
                storeConfirm_=false;
                const bool cancelled=store_.Cancel(item);
                notice_=cancelled?"Solicitacao removida; arquivos locais preservados.":"Falha ao salvar fila.";
                RecordEvent(cancelled?"Pedido da loja removido":"Falha ao remover pedido");
            }
            if(key==Accept) {
                if(!item.provider.empty()){storeConfirm_=false;notice_="Catalogo remoto: use START para baixar no Xbox.";return "";}
                if(!TranslationMatches(item)) {
                    storeConfirm_=false;storeTranslationMatch_=false;
                    notice_="Traducao: nenhum XEX local com Title ID, Media ID e versao iguais.";return "";
                }
                if(!storeConfirm_) {storeConfirm_=true;notice_="A confirma pedido para o PC | B cancela.";}
                else {
                    storeConfirm_=false;
                    const bool requested=store_.Request(item);
                    notice_=requested?"Pedido salvo. Execute o downloader PC.":"Fila cheia ou falha ao gravar.";
                    RecordEvent(requested?"Pedido da loja salvo":"Falha ao salvar pedido");
                }
            }
        }
        return "";
    }
    if(page_==Home && importPreview_) {
        if(key==Back) {importPreview_=false;notice_="Importacao cancelada.";}
        if(key==Accept)ApplyAuroraProfile();
        return "";
    }
    // Edit a draft only; preserve the saved layout until Y succeeds.
    if(page_==Home && homeEditing_) {
        if(layout_==DashboardLayout::TopIcons){
            if(key==Back){homeEditing_=false;notice_.clear();return "";}
            if(key==Left||key==Right)homeEditSlot_=std::max(0,std::min(5,homeEditSlot_+(key==Left?-1:1)));
            if(key==Up||key==Down)homeEditSlot_=(homeEditSlot_+3)%6;
            if(key==PageUp||key==PageDown){int target=homeEditSlot_+(key==PageUp?-1:1);if(target>=0&&target<6){std::swap(approvedDraft_[homeEditSlot_],approvedDraft_[target]);homeEditSlot_=target;}}
            if(key==Favorite){approvedDraft_="012345";homeEditSlot_=0;}
            if(key==Refresh){const auto old=approvedShortcuts_;approvedShortcuts_=approvedDraft_;if(SaveProfile()){homeEditing_=false;notice_="Atalhos salvos.";}else{approvedShortcuts_=old;notice_="Falha ao salvar.";}}
            return "";
        }
        if(key==Back) {homeEditing_=false;notice_="Personalizacao cancelada.";return "";}
        if(key==Left && homeEditSlot_>0) --homeEditSlot_;
        if(key==Right && homeEditSlot_<3) ++homeEditSlot_;
        if(key==PageUp && homeEditSlot_>0) {
            std::swap(homeDraft_[homeEditSlot_],homeDraft_[homeEditSlot_-1]);
            --homeEditSlot_;
        }
        if(key==PageDown && homeEditSlot_<3) {
            std::swap(homeDraft_[homeEditSlot_],homeDraft_[homeEditSlot_+1]);
            ++homeEditSlot_;
        }
        if(key==Favorite) {homeDraft_=DefaultHomeOrder;homeEditSlot_=0;}
        if(key==Refresh) {
            const std::string previous=homeOrder_;
            homeOrder_=homeDraft_;
            if(SaveProfile()) {homeEditing_=false;notice_="Atalhos personalizados e salvos.";}
            else {homeOrder_=previous;notice_="Falha ao salvar. Y tenta novamente; B cancela.";}
        }
        return "";
    }
    if(artEditing_) {
        if(artEditRow_==2){
            if(key==Back){artEditing_=false;notice_.clear();return "";}
            if(key==Up){artEditRow_=1;notice_.clear();return "";}
            if(key==Down){artEditRow_=0;notice_.clear();return "";}
            if(key==PageDown){artDirty_=true;return "";}
            if(key==Refresh){artEditRow_=0;return Press(Refresh);}
            if(key==Search){
                if(TransferRunning()){notice_="Conclua a transferencia ativa.";return "";}
                unsigned title=0;for(char ch:artEditId_)title=(title<<4)|(ch<='9'?ch-'0':ch-'A'+10);
                StopFtp();std::string result;bool ok=ArtImport::Local(fileWorkspace_,dir_,title,true,result,true);ArtResult(ok?"Verso importado.":result,ok);return "";
            }
            return "";
        }
        if(artArchiveConfirm_){
            artArchiveConfirm_=false;
            if(key!=Accept){notice_="Operacao de arte cancelada.";return "";}
            if(TransferRunning()){notice_="Conclua a transferencia ativa.";return "";}
            unsigned title=0;for(char ch:artEditId_)title=(title<<4)|(ch<='9'?ch-'0':ch-'A'+10);
            StopFtp();std::string error;
            const bool ok=ArtImport::SetArchived(fileWorkspace_,dir_,title,artEditRow_==0,!artArchiveRestore_,error);
            artDirty_=true;
            notice_=ok?(artArchiveRestore_?"Arte pessoal restaurada.":"Arte pessoal arquivada; importe outra se desejar."):error;
            RecordEvent(ok?"Arquivo de arte movido":"Movimento de arte recusado");
            return "";
        }
        if(key==PageUp){
            if(TransferRunning()){notice_="Conclua a transferencia ativa.";return "";}
            unsigned title=0;for(char ch:artEditId_)title=(title<<4)|(ch<='9'?ch-'0':ch-'A'+10);
            const auto active=FileWorkspace::Join(ArtImport::Folder(dir_,title),artEditRow_==0?"cover.rgba":"background.rgba");
            const bool have=fileWorkspace_.CanReadPersonalFile(active),saved=ArtImport::HasArchived(fileWorkspace_,dir_,title,artEditRow_==0);
            if(have&&saved){notice_="Arte atual e arquivo existem; preserve um deles pelo explorer.";return "";}
            if(!have&&!saved){notice_="Nenhuma arte pessoal disponivel.";return "";}
            artArchiveRestore_=!have;artArchiveConfirm_=true;
            notice_=artArchiveRestore_?"A confirma restaurar arte; B cancela.":"A confirma arquivar arte; B cancela.";
            return "";
        }
#if defined(INFINITY_XSDK_HTTPS)
        if(key==Guide||key==Search){
            if(TransferRunning()){notice_="Conclua a transferencia ativa.";return "";}
            unsigned title=0;for(char ch:artEditId_)title=(title<<4)|(ch<='9'?ch-'0':ch-'A'+10);
            if(!title){notice_="Title ID invalido.";return "";}
            if(key==Guide&&artEditRow_!=0){notice_="XboxUnity: capa online; fundo via R3 importa JPG/PNG.";return "";}
            artTitle_=title;artCover_=artEditRow_==0;artRemote_=key==Guide;artPending_=true;return "";
        }
#endif
        if(key==Back) {artEditing_=false;notice_="Alteracoes de arte canceladas.";return "";}
        if(key==Up || key==Down)artEditRow_=(artEditRow_+(key==Down?1:2))%3;
        if(key==Left || key==Right || key==Accept) {
            const int delta=key==Left?2:1;
            artDraft_[artEditRow_]=static_cast<char>('0'+(artDraft_[artEditRow_]-'0'+delta)%3);
        }
        if(key==Favorite)artDraft_="00";
        if(key==PageDown) {artDirty_=true;notice_="Arquivos locais recarregados.";}
        if(key==Refresh) {
            const std::map<std::string,std::string>::iterator it=artChoices_.find(artEditId_);
            const bool had=it!=artChoices_.end();
            const std::string old=had?it->second:"00";
            if(artDraft_!="00" && !had && artChoices_.size()>=ArtChoiceLimit) {
                notice_="Limite de 512 personalizacoes.";return "";
            }
            if(artDraft_=="00")artChoices_.erase(artEditId_);
            else artChoices_[artEditId_]=artDraft_;
            if(SaveProfile()) {artEditing_=false;notice_="Arte personalizada e salva.";}
            else {
                if(had)artChoices_[artEditId_]=old;
                else artChoices_.erase(artEditId_);
                notice_="Falha ao salvar. Y tenta novamente; B cancela.";
            }
        }
        return "";
    }
    if(contentActive_) {
        if(contentInstallActive_){
            if(key==Back){if(contentInstallConfirm_)contentInstallConfirm_=false;else contentInstallActive_=false;notice_.clear();return "";}
            if(key==Refresh){RefreshContentInstall();return "";}
#if defined(INFINITY_XSDK_HTTPS)
            if(key==Accept&&contentInstallKnown_&&!TransferRunning()){
                if(!contentInstallConfirm_){contentInstallConfirm_=true;notice_="A confirma instalar conteudo autorizado no HDD; sem sobrescrita.";return "";}
                contentInstallConfirm_=false;contentInstallPending_=true;
            }
#endif
            return "";
        }
        if(!unityActive_&&key==Search&&(contentTab_==0||contentTab_==1||contentTab_==4)&&contentKnown_&&!detailMetadata_.package){
            contentInstallStaged_.clear();contentInstallBackup_.clear();contentInstallActive_=true;RefreshContentInstall();return "";
        }
        if(unityActive_){
            if(key==Back){if(unityConfirm_)unityConfirm_=false;else unityActive_=false;notice_.clear();return "";}
            if(key!=Accept)unityConfirm_=false;
            if(key==Up&&unityRow_>0)--unityRow_;
            if(key==Down&&static_cast<size_t>(unityRow_+1)<unityUpdates_.size())++unityRow_;
#if defined(INFINITY_XSDK_HTTPS)
            if(key==Refresh&&!TransferRunning()){unityPending_=true;notice_="Consultando XboxUnity.";}
            if(key==Accept&&!unityUpdates_.empty()&&!TransferRunning()){
                const auto& selected=unityUpdates_[unityRow_];
                if(selected.url.empty()||!selected.bytes||!contentInfo_.mediaId){notice_="TU sem fonte, tamanho ou Media ID verificavel.";unityConfirm_=false;return "";}
                if(!unityConfirm_){unityConfirm_=true;notice_="A novamente confirma download para staging, sem ativacao.";return "";}
                unityConfirm_=false;unityDownload_=selected;unityDownloadPending_=true;
            }
#endif
            return "";
        }
        if(key==Guide&&contentTab_==0&&contentKnown_&&!detailMetadata_.package&&contentInfo_.titleId&&contentInfo_.baseVersion){
            unityActive_=true;unityUpdates_.clear();unityRow_=0;
#if defined(INFINITY_XSDK_HTTPS)
            if(!TransferRunning()){unityPending_=true;notice_="Consultando XboxUnity.";}
            else notice_="Conclua a transferencia ativa; Y tenta novamente.";
#else
            notice_="Consulta online disponivel no XSDK.";
#endif
            return "";
        }
        if(recoverConfirm_||archiveTuConfirm_||restoreArchivedConfirm_){
            const bool recover=recoverConfirm_,restore=restoreArchivedConfirm_;recoverConfirm_=false;archiveTuConfirm_=false;restoreArchivedConfirm_=false;
#if defined(INFINITY_XSDK_HTTPS)
            if(key==Accept&&!TransferRunning()){
                if(recover){recoverPending_=true;notice_=verifyOnly_?"Verificando SHA-256.":"Recuperando copia verificada.";}
                else{
                    StopFtp();std::string error;
                    if(restore?ContentBackup::RestoreArchived(fileWorkspace_,dir_,backupItem_,error):ContentBackup::ArchiveStaged(fileWorkspace_,dir_,backupItem_,error)){
                        stagedItems_=ContentBackup::Staged(fileWorkspace_,dir_,contentInfo_.titleId);
                        archivedItems_=ContentBackup::Archived(fileWorkspace_,dir_,contentInfo_.titleId);
                        clampRow(contentRow_,ContentCount());
                        notice_=restore?"TU recuperada para downloads; sem ativacao.":"TU arquivada em UserFiles/ArchivedTU; sem exclusao.";
                    }else notice_=error;
                }
                return "";
            }
#endif
            notice_="Operacao cancelada.";return "";
        }
        if(toggleTuConfirm_){
            toggleTuConfirm_=false;
#if defined(INFINITY_XSDK_HTTPS)
            if(key==Accept&&!TransferRunning()){
                StopFtp();std::string error;XboxExecutionInfo current{};
                const bool restore=contentTab_==7;
                if(!Count()||!ContentCatalog::ReadExecutionInfo(visible_[row_]->executable,current)||
                    current.titleId!=contentInfo_.titleId||current.mediaId!=contentInfo_.mediaId||current.baseVersion!=contentInfo_.baseVersion){
                    notice_="Jogo alterado; selecione novamente.";return "";
                }
                if(ContentInstall::SetUpdateAvailable(fileWorkspace_,dir_,current,backupItem_,restore,error)){
                    contentItems_=ContentCatalog(ContentCatalog::DefaultRoots(dir_)).ScanExtras(contentInfo_.titleId);
                    disabledItems_=ContentInstall::DisabledUpdates(fileWorkspace_,dir_,contentInfo_.titleId);
                    clampRow(contentRow_,ContentCount());
                    notice_=restore?"TU recolocada; ativacao nao confirmada.":"TU retirada do local original; arquivo preservado.";
                }else notice_=error;
                return "";
            }
#endif
            notice_="Operacao cancelada.";return "";
        }
        if(backupConfirm_){
            backupConfirm_=false;
            if(key==Accept&&!TransferRunning()){backupPending_=true;notice_="Preparando backup de conteudo.";}
            else notice_="Backup cancelado.";
            return "";
        }
        if(trainerConfirm_) {
            trainerConfirm_=false;
            if(key!=Accept) {notice_="Lancamento cancelado.";return "";}
            if(contentTab_!=3 || static_cast<size_t>(contentRow_)>=trainers_.size() ||
               TransferRunning() || detailMetadata_.package ||
               !Count()) {notice_="Lancamento indisponivel.";return "";}
            const TrainerEntry& trainer=trainers_[contentRow_];
            XboxExecutionInfo current;
            if(!ContentCatalog::ReadExecutionInfo(visible_[row_]->executable,current) ||
               trainer.Compatibility(current)!=TrainerEntry::HeaderMatches ||
               !TrainerCatalog::Recheck(dir_,trainer)) {
                notice_="Trainer ou jogo mudou; atualize e confira a compatibilidade.";return "";
            }
            if(trainer.tuRequirement!=TrainerEntry::Legacy) {
                notice_="TU ativa nao confirmada. Lancamento bloqueado.";return "";
            }
            return trainer.launcher; // Standalone utility only, no injection or automatic game launch.
        }
        if(key==Back) {contentActive_=false;notice_.clear();return "";}
        if(key==Refresh) {const int tab=contentTab_;OpenContent();contentTab_=tab;return "";}
        if(key==Left && contentTab_>0) {--contentTab_;contentRow_=0;}
        if(key==Right && contentTab_<7) {++contentTab_;contentRow_=0;}
        if(key==Up && contentRow_>0)--contentRow_;
        if(key==Down && static_cast<size_t>(contentRow_+1)<ContentCount())++contentRow_;
#if defined(INFINITY_XSDK_HTTPS)
        if(key==Accept&&contentTab_==4&&!TransferRunning()&&!detailMetadata_.package&&static_cast<size_t>(contentRow_)<stagedItems_.size()){
            contentInstallBackup_.clear();contentInstallStaged_=stagedItems_[contentRow_].name;contentInstallActive_=true;RefreshContentInstall();return "";
        }
        if(key==Accept&&contentTab_==6&&!TransferRunning()&&static_cast<size_t>(contentRow_)<archivedItems_.size()){
            backupItem_=archivedItems_[contentRow_];restoreArchivedConfirm_=true;notice_="A confirma recuperar TU arquivada; B cancela.";return "";
        }
        if(key==Search&&contentTab_==5&&!TransferRunning()&&!detailMetadata_.package&&static_cast<size_t>(contentRow_)<backups_.size()){
            contentInstallStaged_.clear();contentInstallBackup_=backups_[contentRow_].slot;contentInstallActive_=true;RefreshContentInstall();return "";
        }
        if((key==Accept||key==Favorite)&&contentTab_==5&&!TransferRunning()&&static_cast<size_t>(contentRow_)<backups_.size()){
            verifyOnly_=key==Favorite;recoverItem_=backups_[contentRow_];recoverConfirm_=true;notice_=verifyOnly_?"A confirma verificar SHA-256 sem escrita.":"A confirma recuperar para area pessoal, sem ativacao.";return "";
        }
        if(((key==Favorite&&contentTab_==0)||(key==Accept&&contentTab_==7))&&!TransferRunning()&&!detailMetadata_.package){
            const auto* item=ContentAt(contentRow_);
            if(item){backupItem_=*item;toggleTuConfirm_=true;notice_=contentTab_==7?"A confirma recolocar TU; B cancela.":"A confirma retirar TU do local original; B cancela.";}
            return "";
        }
        if(key==Favorite&&contentTab_==4&&!TransferRunning()&&static_cast<size_t>(contentRow_)<stagedItems_.size()){
            backupItem_=stagedItems_[contentRow_];archiveTuConfirm_=true;notice_="A confirma arquivar TU baixada; B cancela.";return "";
        }
        if(key==PageDown&&contentTab_!=3&&contentTab_!=5&&!TransferRunning()){
            const auto* item=ContentAt(contentRow_);
            if(item){backupItem_=*item;backupConfirm_=true;notice_="A confirma backup sem modificar a origem; B cancela.";}
            return "";
        }
#endif
        if(key==Accept && contentTab_==3 && static_cast<size_t>(contentRow_)<trainers_.size()) {
            if(TransferRunning() || detailMetadata_.package ||
               trainers_[contentRow_].Compatibility(contentInfo_)!=TrainerEntry::HeaderMatches) {
                notice_="Lancamento bloqueado: formato, transferencia ou versao incompativel.";
            } else if(trainers_[contentRow_].tuRequirement!=TrainerEntry::Legacy) {
                const TrainerEntry::TuCheck check=trainers_[contentRow_].CheckUpdates(contentItems_);
                notice_=check==TrainerEntry::RequiredMissing?"TU exigida nao encontrada no HD/USB.":
                    check==TrainerEntry::RequiredAmbiguous?"Multiplas TUs candidatas; estado ativo desconhecido.":
                    check==TrainerEntry::RequiredLocal?"TU exigida localizada; ativacao nao confirmada.":
                    check==TrainerEntry::UnnumberedLocal?"TU candidata sem numero confirmado; ativacao desconhecida.":
                    "Trainer sem TU: estado ativo desconhecido.";
            } else {
                trainerConfirm_=true;
                notice_="A confirma utilitario local confiavel/offline; B cancela.";
            }
        } else if(key==Accept) {
            const XboxContentPackage* item=ContentAt(contentRow_);
            if(item)notice_=item->path;
        }
        return "";
    }
    if(page_==Details&&(section_==Library||section_==Recent||section_==Favorites)&&key==Search&&Count()){
        if(!DiscSet::ReadIdentity(visible_[row_]->executable,discSetIdentity_)||discSetIdentity_.discCount<2){
            notice_="Nenhum conjunto multidisco identificado.";return "";
        }
        discSet_=DiscSet::Find(games_,discSetIdentity_);discSetRow_=0;discSetActive_=true;notice_.clear();return "";
    }
    if(page_==Details && (section_==Library || section_==Recent || section_==Favorites)
       && key==Guide && Count()) {
        artEditFolder_=ArtFolder(*visible_[row_]);
        if(artEditFolder_.empty()) {notice_="Arte indisponivel: Title ID desconhecido.";return "";}
        artEditId_=artEditFolder_.substr(artEditFolder_.size()-9,8);
        const std::map<std::string,std::string>::const_iterator it=artChoices_.find(artEditId_);
        artDraft_=it==artChoices_.end()?"00":it->second;
        artEditRow_=0;artEditing_=true;artArchiveConfirm_=false;notice_.clear();
        return "";
    }
    if(page_==Details && (section_==Library || section_==Recent || section_==Favorites)
       && key==PageDown && Count()) {
        OpenContent();
        return "";
    }
    if((section_==Library || section_==Files) && page_==Listing && searchActive_) {
        if(key==Back) {
            if(keyboardMode_==0) searchDraft_=searchTerm_;
            searchActive_=false;
            keyboardMode_=0;
            return "";
        }
        if(key==Left && searchCursor_%7>0) --searchCursor_;
        if(key==Right && searchCursor_%7<6) ++searchCursor_;
        if(key==Up && searchCursor_>=7) searchCursor_-=7;
        if(key==Down && searchCursor_+7<KeyCount) searchCursor_+=7;
        if(key==Favorite && !searchDraft_.empty()) searchDraft_.erase(searchDraft_.size()-1);
        if(key==Refresh) ApplySearch();
        if(key==Accept) {
            if(searchCursor_<36) {
                if(searchDraft_.size()<36) searchDraft_+=keyboardKeys[searchCursor_];
            } else if(searchCursor_==36) {
                if(searchDraft_.size()<36) searchDraft_+=" ";
            } else if(searchCursor_==37) {
                if(!searchDraft_.empty()) searchDraft_.erase(searchDraft_.size()-1);
            } else if(searchCursor_==38) ApplySearch();
            else if(searchCursor_==39) searchDraft_.clear();
            else if(searchCursor_==40) {
                if(keyboardMode_==0) searchDraft_=searchTerm_;
                searchActive_=false;
                keyboardMode_=0;
            } else if(searchCursor_==41 && searchDraft_.size()<36) searchDraft_+=".";
        }
        return "";
    }
    if(section_==Library && page_==Listing && collectionPicker_) {
        const size_t count=collections_.Items().size();
        if(key==Back) {
            collectionPicker_=false;
            deleteCollectionConfirm_=false;
            notice_.clear();
            return "";
        }
        if(key==Up && collectionRow_>0) --collectionRow_;
        if(key==Down && static_cast<size_t>(collectionRow_)<count)
            ++collectionRow_; // last row: create new
        if(key==Favorite && static_cast<size_t>(collectionRow_)<count) {
            collectionIndex_=collectionRow_;
            searchDraft_=collections_.Items()[collectionRow_].name;
            searchActive_=true;
            keyboardMode_=2;
            searchCursor_=0;
            deleteCollectionConfirm_=false;
            notice_.clear();
            return "";
        }
        if(key==Refresh && static_cast<size_t>(collectionRow_)<count) {
            if(!deleteCollectionConfirm_) {
                deleteCollectionConfirm_=true;
                notice_="Pressione Y novamente para excluir a colecao (nao os jogos).";
            } else {
                collections_.Remove(static_cast<size_t>(collectionRow_));
                const bool saved=collections_.Save();
                const size_t remaining=collections_.Items().size();
                if(collectionRow_>static_cast<int>(remaining))
                    collectionRow_=static_cast<int>(remaining);
                collectionIndex_=remaining?
                    std::min(collectionIndex_,static_cast<int>(remaining-1)):0;
                collectionPicker_=false;
                deleteCollectionConfirm_=false;
                subtab_=5;
                row_=0;
                Rebuild();
                notice_=saved?"Colecao excluida (jogos preservados).":
                    "Colecao retirada desta sessao; falha ao salvar.";
            }
            return "";
        }
        if(key==Accept) {
            if(static_cast<size_t>(collectionRow_)==count) {
                keyboardMode_=1;
                searchDraft_.clear();
                searchCursor_=0;
                searchActive_=true;
                deleteCollectionConfirm_=false;
                notice_.clear();
                return "";
            }
            if(static_cast<size_t>(collectionRow_)<count) {
                collectionIndex_=collectionRow_;
                bool saved=true,modified=false;
                if(!pickerGame_.empty()) {
                    modified=collections_.Toggle(
                        static_cast<size_t>(collectionIndex_),pickerGame_);
                    if(modified) saved=collections_.Save();
                }
                collectionPicker_=false;
                deleteCollectionConfirm_=false;
                subtab_=5;
                row_=0;
                Rebuild();
                notice_=!modified && !pickerGame_.empty()?
                    "Limite de itens atingido nesta colecao.":
                    !saved?"Colecao atualizada apenas nesta sessao.":
                    "Colecao selecionada / jogo atualizado.";
                return "";
            }
        }
        if(key!=Refresh) deleteCollectionConfirm_=false;
        return "";
    }
    if(section_==Files && page_==Listing) {
        if(fileAction_) {
            if(key==Back) {fileAction_=false;deleteConfirm_=false;notice_.clear();return "";}
            if(deleteConfirm_) {
                deleteConfirm_=false;
                if(key==Accept && !TransferRunning()) {
                    const bool removed=fileWorkspace_.Remove(actionDir_,actionName_);
                    fileAction_=false;Reload();
                    notice_=removed?"Item excluido.":"Exclusao recusada: pasta nao vazia ou acesso invalido.";
                } else notice_="Exclusao cancelada.";
                return "";
            }
            if(key==Up && fileActionRow_>0)--fileActionRow_;
            if(key==Down && fileActionRow_<3)++fileActionRow_;
            if(key==Accept) {
                if(fileActionRow_==3) {fileAction_=false;return "";}
                if(fileActionRow_==2) {
                    multiSelect_=!(multiSelect_ && copyDir_==actionDir_);
                    copyDir_=actionDir_;copyNames_.clear();copyName_.clear();cutMode_=false;
                    if(multiSelect_)copyNames_.push_back(actionName_);
                    fileAction_=false;notice_=multiSelect_?"Multisselecao: X marca/desmarca; LB cola.":"Selecao limpa.";
                    return "";
                }
                if(TransferRunning()||!fileWorkspace_.CanWrite(actionDir_)) {
                    notice_="Operacao limitada ao espaco pessoal, sem transferencias ativas.";return "";
                }
                if(fileActionRow_==0) {
                    if(!fileWorkspace_.CanReadPersonalFile(FileWorkspace::Join(actionDir_,actionName_))) {
                        notice_="Recortar disponivel somente para arquivos pessoais.";return "";
                    }
                    copyDir_=actionDir_;copyName_=actionName_;cutMode_=true;multiSelect_=false;copyNames_.clear();
                    fileAction_=false;notice_="Arquivo recortado. LB cola no destino pessoal.";
                } else {deleteConfirm_=true;notice_="A confirma exclusao definitiva | B cancela.";}
            }
            return "";
        }
        if(scanPathConfirm_) {
            scanPathConfirm_=false;
            if(key==Accept)ToggleScanPath();
            else notice_="Alteracao de caminho cancelada.";
            return "";
        }
        if(pasteConfirm_) {
            pasteConfirm_=false;
            if(key==Accept) {
                if(cutMode_) {
                    if(TransferRunning())notice_="Conclua a fila antes de mover arquivos.";
                    else if(fileWorkspace_.Move(copyDir_,copyName_,folders_.back())) {
                        copyName_.clear();copyDir_.clear();cutMode_=false;
                        Reload();notice_="Arquivo movido sem sobrescrever.";
                    } else notice_="Movimento recusado; confira destino e origem.";
                } else EnqueueCopy();
            } else notice_="Copia cancelada.";
            return "";
        }
        if(key==Right) {
            if(Count()) {
                actionDir_=folders_.back();actionName_=files_[row_].name;
                fileActionRow_=0;fileAction_=true;deleteConfirm_=false;notice_.clear();
            }
            return "";
        }
        if(key==Left) {
            if(folders_.size()<2 || TransferRunning())notice_="Abra uma pasta; conclua transferencias antes de editar caminhos.";
            else {scanPathConfirm_=true;notice_="A alterna pasta atual na busca personalizada; B cancela.";}
            return "";
        }
        if(key==PageDown) {
            fileRoot_=(fileRoot_+1)%fileWorkspace_.Roots().size();
            folders_.clear();folders_.push_back(fileWorkspace_.Roots()[fileRoot_]);
            row_=0;Reload();return "";
        }
        if(key==Guide || key==Search) {
            if(TransferRunning())notice_="Conclua ou cancele a transferencia antes de editar arquivos.";
            else StartFileEdit(key==Search);
            return "";
        }
        if(key==Favorite) {
            if(Count() && FileWorkspace::ValidName(files_[row_].name)) {
                const std::string name=files_[row_].name;
                if(multiSelect_ && copyDir_==folders_.back()) {
                    std::vector<std::string>::iterator found=std::find(copyNames_.begin(),copyNames_.end(),name);
                    if(found!=copyNames_.end())copyNames_.erase(found);
                    else if(copyNames_.size()<32)copyNames_.push_back(name);
                    else {notice_="Limite de 32 itens selecionados.";return "";}
                    notice_=number(copyNames_.size())+" itens selecionados.";
                } else {
                    copyDir_=folders_.back();copyName_=name;cutMode_=false;
                    multiSelect_=false;copyNames_.clear();
                    notice_="Item marcado para copiar: "+copyName_;
                }
            } else notice_="Selecione um item com nome valido.";
            return "";
        }
        if(key==PageUp) {
            if((copyName_.empty() && copyNames_.empty()) || !fileWorkspace_.CanWrite(folders_.back()))
                notice_="Marque um arquivo com X; abra UserFiles/AuroraXFiles para colar.";
            else {pasteConfirm_=true;notice_=cutMode_?"A confirma mover sem sobrescrever; B cancela.":"A confirma a copia sem sobrescrever; B cancela.";}
            return "";
        }
    }
    if(section_==Iptv && page_!=Home) {
        if(!tv_.Press(static_cast<MenuKey>(key))) {page_=Home;notice_.clear();}
        return "";
    }
    if(appearanceEditor_&&section_==System&&systemTab_==7&&page_==Details){PressAppearance(key);return "";}
    if(achievementsPage_&&section_==System&&systemTab_==7&&page_==Details){PressAchievements(key);return "";}
    if(saveCenter_&&section_==System&&systemTab_==7&&page_==Details){PressSaveCenter(key);return "";}
    // Modal power panel consumes every input; Back cancels before leaving.
    if(section_==System&&systemTab_==7&&page_==Details&&powerPage_){
        if(key==Back){
            if(powerConfirm_.armed)powerConfirm_.Cancel();
            else powerPage_=false;
            notice_.clear();return "";
        }
        if(key==Left||key==Right){
            powerConfirm_.Select(key==Left?ConsolePower::Shutdown:ConsolePower::Reboot);
            notice_.clear();return "";
        }
        if(key==Accept){
            const auto step=powerConfirm_.Accept(ConsolePower::Supported(),PowerBusy());
            if(step==ConsolePower::Unavailable)notice_="Provider experimental nao incluido.";
            else if(step==ConsolePower::Busy)notice_="Encerre FTP, testes de rede e transferencias primeiro.";
            else if(step==ConsolePower::Armed)notice_="A novamente confirma | B cancela.";
            else if(step==ConsolePower::Requested){
                const auto result=ConsolePower::Invoke(powerConfirm_.action);
                notice_=result==ConsolePower::Busy?"Falha ao descarregar buffers; operacao cancelada.":"Comando retornou ou indisponivel; energia nao confirmada.";
            }
        }else powerConfirm_.Cancel();
        return "";
    }
    if(section_==System&&systemTab_==1&&page_==Details&&smbPage_){
        if(key==Back){if(smbEditing_)smbEditing_=false;else{smbProbe_.Stop();smbPage_=false;}notice_.clear();return "";}
        if(smbEditing_){
            if(key==Left&&smbOctet_>0)--smbOctet_;
            if(key==Right&&smbOctet_<3)++smbOctet_;
            if(key==Up||key==Down)LanPeerConfig::Change(smbDraft_,smbOctet_,key==Up?1:-1);
            if(key==Accept){
                RefreshNetwork();
                if(LanPeerConfig::Format(smbDraft_)==network_.ipv4||!LanPeerConfig::Save(dir_+"infinity360-smb-peer.txt",smbDraft_))notice_="IPv4 privado distinto ou gravacao invalida.";
                else{smbEditing_=false;smbProbe_.Reset();notice_="Servidor SMB salvo.";}
            }
            return "";
        }
        if(key==Favorite){smbProbe_.Stop();return "";}
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMB_MENU)
        if(key==Search&&!smbProbe_.Running()){
            if(PowerBusy()||!SmbEnabled()){notice_="Encerre operacoes e habilite enable-smb-session.txt.";return "";}
            CloseSmbSession();smbUi_=SmbCredentials;smbField_=smbChar_=0;notice_.clear();return "";
        }
#endif
        if(key==Guide&&!smbProbe_.Running()){
            if(TransferRunning()||ftp_.Running()){notice_="Encerre FTP e transferencias antes de editar.";return "";}
            smbDraft_={{192,168,1,2}};LanPeerConfig::Read(dir_+"infinity360-smb-peer.txt",smbDraft_);
            smbOctet_=0;smbEditing_=true;notice_.clear();return "";
        }
        if(key==Accept&&!smbProbe_.Running()){
            if(PowerBusy()){notice_="Encerre operacoes e FTP antes do teste SMB.";return "";}
            LanPeerConfig::Octets configured{};
            if(!LanPeerConfig::Read(dir_+"infinity360-smb-peer.txt",configured)){notice_="START configura IPv4 do servidor.";return "";}
            RefreshNetwork();smbProbe_.Start(network_.ipv4,LanPeerConfig::Format(configured));notice_.clear();
        }
        return "";
    }
    if (key==Back) {
        if(ApprovedHeader()&&ApprovedHome::CoverSlot(focus_)){PressApprovedHome(key);return "";}
        if(section_==System&&systemTab_==1&&page_==Details){
            if(peerEditing_||peerOptions_){peerEditing_=false;peerOptions_=false;notice_.clear();return "";}
            peerProbe_.Stop();
        }
        notice_.clear();
        if (page_==Details) {
            page_=Listing;
            if(section_==Recent) { Rebuild(); row_=0; }
            else if(section_==Library && subtab_==6)Rebuild();
        }
        else if (page_==Listing && section_==Files && folders_.size()>1) {
            folders_.pop_back(); row_=0; Reload();
        } else page_=Home;
        return "";
    }
    if(key==Refresh) {
        if(ApprovedHeader()){PressApprovedHome(key);return "";}
        if(section_==System && page_!=Home) {
            if(systemTab_==0) {RefreshStorage();notice_="Armazenamento consultado novamente.";}
            else if(systemTab_==1) {RefreshNetwork();notice_="Diagnostico de rede atualizado.";}
            else if(systemTab_==2) {if(thermalPage_){notice_="A consulta sensores; leitura experimental manual.";}else{telemetry_=SystemTelemetry::Read();videoTelemetry_=VideoTelemetry::Read();notice_="Telemetria atualizada.";}}
            else if(systemTab_==3) {
                if(storeDownloads_) {store_.Load();RebuildStoreDownloads();notice_="Registros de downloads atualizados.";}
                else notice_="Transferencias locais atualizadas automaticamente.";
            }
            else if(systemTab_==4) {RefreshResidentPlugins();RefreshLocalPlugins();notice_="Configuracao de plugins consultada.";}
            else if(systemTab_==5){clampRow(eventRow_,notifications_.Visible(eventFilter_).size());notice_="Historico local dos ultimos 64 eventos.";}
            else if(systemTab_==7){status_=ConsoleStatus::Read();notice_="Perfis e configuracoes consultados.";}
            else notice_="Este servico ainda nao possui provider nativo.";
            return "";
        }
        if(page_==Home&&clockSettings_.city)weatherPending_=true;
        if(page_==Details) page_=Listing;
        Reload();
        if(notice_.empty()) notice_="Atualizado";
        return "";
    }
    if(section_==System && page_!=Home) {
        if(page_==Details&&systemTab_==5){
            if(key==PageUp||key==PageDown){
                const unsigned count=NotificationCenter::FilterCount;
                eventFilter_=static_cast<NotificationCenter::Filter>((eventFilter_+count+(key==PageDown?1:count-1))%count);
                eventRow_=0;notice_.clear();
            }else if(key==Accept||key==Favorite)MarkNotifications(key==Favorite);
            else if(key==Up||key==Down){eventRow_=std::max(0,eventRow_+(key==Down?1:-1));clampRow(eventRow_,notifications_.Visible(eventFilter_).size());}
            return "";
        }
        if(page_==Details&&systemTab_==7){
            if(key==Up||key==Down){localMilestones_=!localMilestones_;return "";}
            if(localMilestones_){if(key==Guide){saveCenter_=true;RefreshSaveProfiles();notice_.clear();}else if(key==Accept){achievementsPage_=true;RefreshAchievements();}else if(key==Favorite){appearanceDraft_=appearance_;clockDraft_=clockSettings_;themeDraft_=theme_;layoutDraft_=layout_;gridDraft_=gridPreference_;layoutPreview_=false;appearanceRow_=0;appearanceEditor_=true;wallpaperDirty_=true;notice_.clear();}return "";}
            if(key==Accept){powerPage_=true;powerConfirm_=ConsolePower::Confirmation();notice_.clear();return "";}
            if(key==PageUp||key==PageDown){const auto previous=language_;
                language_=(language_+(key==PageDown?1:UiLanguage::Count-1))%UiLanguage::Count;
                if(!SaveProfile()){language_=previous;notice_="Falha ao salvar idioma.";}
                else notice_="Idioma da interface salvo.";return ""; }
            if(key==Favorite){const auto previous=idleMinutes_;idleMinutes_=IdleScreen::Next(idleMinutes_);
                if(!SaveProfile()){idleMinutes_=previous;notice_="Falha ao salvar protetor de tela.";}
                else notice_="Protetor de tela salvo.";return ""; }
            if(key==Search){updateActive_=true;RefreshDashboardUpdate();return "";}
#if defined(INFINITY_XSDK_HTTPS)
            if(key==Guide&&!TransferRunning()){
                if(!coverBatchConfirm_){
                    PrepareCoverBatch();coverBatchConfirm_=!coverBatch_.empty();
                    notice_=coverBatchConfirm_?"START confirma baixar "+number(coverBatch_.size())+" capas faltantes. B cancela.":"Nenhuma capa faltante neste lote.";
                }else{coverBatchConfirm_=false;coverBatchPending_=true;}
                return "";
            }
#endif
            if(key==Left||key==Right){
                const auto previous=theme_;theme_=(theme_+(key==Right?1:HomeCanvas::ThemeCount-1))%HomeCanvas::ThemeCount;
                if(!SaveProfile()){theme_=previous;notice_="Falha ao salvar tema.";}
                else notice_="Tema salvo.";
                return "";
            }
        }
        if(page_==Details && systemTab_==6){
#if defined(INFINITY_DISC_INSTALL)
            if(key==Left||key==Right){
                discMode_=(discMode_+(key==Right?1:DiscMethodCount-1))%DiscMethodCount;discConfirm_=false;notice_.clear();
            }
            if((discMode_==ExtractDvd||discMode_==ConvertGod)&&(key==Up||key==Down)){
                discDestination_=(discDestination_+(key==Down?1:3))%4;RefreshDiscInstall();
            }
            if(key==Accept){
                if(TransferRunning()){notice_="Conclua ou cancele a copia ativa.";return "";}
                if(discMode_==ImportPackage){
                    PackageLaunch::Ticket current;std::string error;
                    if(!PackageImport::Read(fileWorkspace_,dir_,current,error)){discConfirm_=false;notice_=error;return "";}
                    if(!discConfirm_||!PackageLaunch::Same(packageImportTicket_,current)){packageImportTicket_=current;discConfirm_=true;notice_="A confirma importar "+current.package.name+"; original preservado.";}
                    else{discConfirm_=false;packageImportPending_=true;}
                    return "";
                }
                if(discMode_==OpenSystem){
                    if(!discConfirm_){discConfirm_=true;notice_="A confirma sair para a dashboard do sistema.";}
                    else{discConfirm_=false;discSystemExit_=true;}
                    return "";
                }
                XboxExecutionInfo current{};
                if(!discKnown_||!discVolume_.available||!ContentCatalog::ReadExecutionInfo("dvd:/default.xex",current)||
                   !DiscIdentity::Same(current,discIdentity_)){
                    RefreshDiscInstall();notice_="DVD ou destino indisponivel/alterado. Confira novamente.";return "";
                }
                if(!discConfirm_){
                    pendingDisc_.format=discMode_==ConvertGod?DiscInstallRequest::Format::God:DiscInstallRequest::Format::Extracted;
                    pendingDisc_.identity=current;pendingDisc_.mount=discVolume_.path;
                    discConfirm_=true;
                    notice_=std::string(discMode_==ConvertGod?"A confirma converter DVD em GOD em ":"A confirma extrair DVD em ")+pendingDisc_.mount;
                }else{discConfirm_=false;discPending_=true;notice_="Preparando instalacao DVD.";}
            }
#endif
            return "";
        }
        if(page_==Details && systemTab_==0 && (key==PageUp||key==PageDown)) {
            const size_t pages=(storageVolumes_.size()+3)/4;
            if(pages)storagePage_=(storagePage_+pages+(key==PageDown?1:pages-1))%pages;
            return "";
        }
        if(page_==Details&&systemTab_==1&&!networkPeer_&&key==Guide){
            smbPage_=true;smbEditing_=false;notice_.clear();return "";
        }
        if(page_==Details&&systemTab_==1&&networkPeer_&&peerEditing_){
            if(key==Left&&peerOctet_>0)--peerOctet_;
            if(key==Right&&peerOctet_<3)++peerOctet_;
            if(key==Up||key==Down)LanPeerConfig::Change(peerDraft_,peerOctet_,key==Up?1:-1);
            if(key==Accept){
                if(LanPeerConfig::Format(peerDraft_)==network_.ipv4)notice_="Escolha o IP do outro console.";
                else if(!LanPeerConfig::Save(dir_+"infinity360-lan-peer.txt",peerDraft_))notice_="IP privado invalido ou falha ao salvar.";
                else{peerEditing_=false;peerProbe_.Reset();notice_="Endereco do par salvo.";}
            }
            return "";
        }
        if(page_==Details&&systemTab_==1&&networkPeer_&&peerOptions_){
            if(key==Left)peerMode_=(peerMode_+2)%3;
            if(key==Right)peerMode_=(peerMode_+1)%3;
            if(key==Up)peerDuration_=(peerDuration_+2)%3;
            if(key==Down)peerDuration_=(peerDuration_+1)%3;
            if(key==Accept){peerOptions_=false;peerProbe_.Reset();notice_="Opcoes aplicadas nesta sessao.";}
            return "";
        }
        if(page_==Details&&systemTab_==1&&(key==PageUp||key==PageDown)){peerProbe_.Stop();peerEditing_=false;peerOptions_=false;networkPeer_=!networkPeer_;return "";}
        if(page_==Details&&systemTab_==1&&networkPeer_&&key==Search){
            if(peerProbe_.Running()){notice_="Encerre o teste antes de mudar opcoes.";return "";}
            peerOptions_=true;notice_.clear();return "";
        }
        if(page_==Details&&systemTab_==1&&networkPeer_&&key==Guide){
            if(peerProbe_.Running()||TransferRunning()){notice_="Encerre o teste e as transferencias antes de editar.";return "";}
            peerDraft_={{192,168,1,2}};LanPeerConfig::Read(dir_+"infinity360-lan-peer.txt",peerDraft_);
            peerOctet_=0;peerEditing_=true;notice_.clear();return "";
        }
        if(page_==Details&&systemTab_==1&&networkPeer_&&(key==Accept||key==Favorite)){
            if(key==Favorite||peerProbe_.Running()){peerProbe_.Stop();return "";}
            if(TransferRunning()){notice_="Conclua transferencias antes do teste.";return "";}
            LanPeerConfig::Octets configured{};
            if(!LanPeerConfig::Read(dir_+"infinity360-lan-peer.txt",configured)){notice_="START configura o IPv4 privado do outro console.";return "";}
            RefreshNetwork();
            if(!LanPeerProbe::Private(network_.ipv4)||network_.ipv4==LanPeerConfig::Format(configured)){notice_="IPv4 local ou par invalido.";return "";}
            const unsigned durations[]={15,30,60};
            StopFtp();peerProbe_.Start(network_.ipv4,LanPeerConfig::Format(configured),static_cast<LanPeerProbe::Mode>(peerMode_),durations[peerDuration_]);notice_.clear();return "";
        }
        if(page_==Details && systemTab_==1 && (key==Accept||key==Favorite)) {ToggleFtp(key==Favorite);return "";}
        if(page_==Details&&systemTab_==2&&key==Guide){thermalPage_=!thermalPage_;coolingPage_=false;return "";}
        if(page_==Details&&systemTab_==2&&thermalPage_){
            if(key==PageUp||key==PageDown){coolingPage_=!coolingPage_;notice_.clear();return "";}
            if(coolingPage_&&(key==Left||key==Right)){
                thermalLimit_=ThermalAdvisory::Next(thermalLimit_,key==Right?1:-1);
                notice_="Limite de aviso desta sessao atualizado.";return "";
            }
            if(key==Accept){
                if(TransferRunning()){notice_="Conclua transferencias antes da consulta.";return "";}
                thermal_=ThermalTelemetry::Read();thermalHistory_.Add(thermal_);
                notice_=ThermalAdvisory::Evaluate(thermal_,thermalLimit_)==ThermalAdvisory::Reached
                    ?"Atencao: limite de aviso escolhido atingido.":"Consulta termica concluida; confira disponibilidade.";
            }else if(key==Favorite){
                thermalHistory_.Clear();thermal_=ThermalTelemetry();
                notice_="Historico termico da sessao limpo.";
            }
            return "";
        }
        if(page_==Details && systemTab_==2 && (key==PageUp||key==PageDown)){telemetryVideo_=!telemetryVideo_;return "";}
        if(page_==Details&&systemTab_==2&&telemetryVideo_&&(key==Left||key==Right||key==Favorite)){
            const unsigned old=safeArea_;
            if(key==Favorite)safeArea_=0;
            else if(key==Left&&safeArea_>0)--safeArea_;
            else if(key==Right&&safeArea_<DisplaySafeArea::Max)++safeArea_;
            if(safeArea_!=old&&!SaveProfile()){safeArea_=old;notice_="Falha ao salvar margem.";}else notice_="Margem de tela: "+number(safeArea_)+"% por lado.";
            return "";
        }
        if(page_==Details&&systemTab_==4&&key==Guide){localPlugins_=!localPlugins_;if(localPlugins_)RefreshLocalPlugins();return "";}
        if(page_==Details&&systemTab_==4&&localPlugins_){
            if(key==Up||key==Down){localPluginRow_=std::max(0,localPluginRow_+(key==Down?1:-1));clampRow(localPluginRow_,localPluginItems_.size());}
            if(key==Favorite)ToggleLocalPlugin();return "";
        }
        if(page_==Details && systemTab_==4 && (key==PageUp||key==PageDown)){
            const size_t count=residentConfigs_.size();
            if(count)residentSource_=(residentSource_+count+(key==PageDown?1:count-1))%count;
            return "";
        }
        if(page_==Details && systemTab_==3 && key==PageDown) {
            storeDownloads_=!storeDownloads_;storeDownloadRow_=0;
            if(storeDownloads_) {store_.Load();RebuildStoreDownloads();}
            return "";
        }
        if(page_==Details && systemTab_==3 && storeDownloads_) {
            if(key!=PageUp)pruneQueueConfirm_=false;
            const size_t count=storeDownloadItems_.size();
            if(key==Up&&storeDownloadRow_>0)--storeDownloadRow_;
            if(key==Down&&static_cast<size_t>(storeDownloadRow_+1)<count)++storeDownloadRow_;
#if defined(INFINITY_XSDK_HTTPS)
            if(key==PageUp){
                if(!store_.NativeQueueValid()){notice_="Fila invalida; arquivo preservado.";return "";}
                if(!store_.OrphanNativeCount()){notice_="Nenhuma referencia ausente.";return "";}
                if(!pruneQueueConfirm_){pruneQueueConfirm_=true;notice_="LB confirma remover referencias ausentes, inclusive caches offline. Parciais preservados.";return "";}
                pruneQueueConfirm_=false;
                notice_=store_.PruneOrphanNative()?"Referencias removidas; arquivos preservados.":"Falha ao salvar fila.";
                RebuildStoreDownloads();return "";
            }
            if(key==Search&&count){
                if(TransferRunning()||nativeStorePending_||nativeBatch_){notice_="Conclua a transferencia ativa.";return "";}
                const auto item=store_.Items()[storeDownloadItems_[storeDownloadRow_]];
                notice_=store_.PrioritizeNative(item)?"Prioridade salva; item no inicio da fila.":"Item fora da fila ou falha ao salvar.";
                RebuildStoreDownloads();
                for(size_t i=0;i<storeDownloadItems_.size();++i)
                    if(store_.Items()[storeDownloadItems_[i]].id==item.id){storeDownloadRow_=static_cast<int>(i);break;}
                return "";
            }
            if(key==Guide){
                nativeBatch_=true;
                if(!NextNativeStore())nativeBatch_=false;
                return "";
            }
            if(count&&(key==Accept||key==Favorite)){
                const StoreItem item=store_.Items()[storeDownloadItems_[storeDownloadRow_]];
                nativeBatch_=false;
                if(key==Accept)StartNativeStore(item);
                else{
                    if(store_.NativeQueued(item))notice_=store_.RemoveNative(item)?"Removido da fila; parcial preservado.":"Falha ao salvar fila.";
                    else notice_="Item fora da fila nativa. Arquivos preservados.";
                    RebuildStoreDownloads();
                }
            }
#endif
            return "";
        }
        if(page_==Details && systemTab_==3 && key==Favorite && TransferRunning()) {
            transfer_.Cancel();copyQueue_.clear();activeCopyTarget_.clear();copyPaused_=false;
            notice_="Fila cancelada; origens preservadas.";RecordEvent("Fila de copias cancelada");return "";
        }
        if(page_==Details && systemTab_==3 && key==Accept && TransferRunning()) {
            copyPaused_=!copyPaused_;copyRate_.Reset();
            if(!copyPaused_)copyRate_.Sample(transfer_.Done(),transfer_.Total(),TransferRate::Now());
            notice_=copyPaused_?"Fila pausada. A para continuar.":"Fila retomada.";
            RecordEvent(copyPaused_?"Fila pausada":"Fila retomada");return "";
        }
        // Real console panels: do not invent temperature/network/FTP readings.
        if(page_==Listing) {
            if(key==Left && systemTab_>0) --systemTab_;
            if(key==Right && systemTab_<7) ++systemTab_;
            if(key==Up&&systemTab_%2==1)--systemTab_;
            if(key==Down&&systemTab_%2==0)++systemTab_;
            if(key==Accept) {
                if(systemTab_==0)RefreshStorage();
                else if(systemTab_==1){RefreshNetwork();networkPeer_=false;peerEditing_=false;peerOptions_=false;peerProbe_.Stop();smbPage_=false;smbEditing_=false;smbProbe_.Stop();}
                else if(systemTab_==2){telemetry_=SystemTelemetry::Read();videoTelemetry_=VideoTelemetry::Read();telemetryVideo_=false;thermalPage_=false;coolingPage_=false;}
                else if(systemTab_==4){localPlugins_=false;RefreshResidentPlugins();RefreshLocalPlugins();}
                else if(systemTab_==5){eventRow_=0;eventFilter_=NotificationCenter::All;}
                else if(systemTab_==6){discMode_=ExtractDvd;discDestination_=0;RefreshDiscInstall();}
                else if(systemTab_==7){localMilestones_=false;saveCenter_=false;achievementsPage_=false;appearanceEditor_=false;powerPage_=false;powerConfirm_.Cancel();status_=ConsoleStatus::Read();}
                else if(systemTab_==3) {storeDownloads_=false;storeDownloadRow_=0;}
                page_=Details;
            }
        }
        return "";
    }
    if(page_==Home) {
        if(layout_==DashboardLayout::TopIcons&&PressApprovedHome(key))return "";
        static const int next[][4]={
            {0,1,0,2},{0,1,1,4},{2,3,0,6},{2,4,0,6},
            {3,5,1,7},{4,5,1,7},{6,7,2,6},{6,7,4,7}
        };
        if(key<=Down) { focus_=layout_==DashboardLayout::TopIcons?ApprovedHome::Move(focus_,key,static_cast<int>(ApprovedShelf().size())):layout_?DashboardLayout::Move(DashboardLayout::Get(layout_),focus_,key):next[focus_<8?focus_:0][key]; notice_.clear(); }
        if(key==Search){quickGuideOpen_=true;quickGuideEditing_=false;quickGuideRow_=0;socialGuideFocus_=false;socialGuideRow_=0;notice_.clear();return "";}
        if(key==PageUp){section_=System;systemTab_=5;page_=Details;eventFilter_=NotificationCenter::All;eventRow_=0;notice_.clear();return "";}
        if(key==PageDown) {
            storeActive_=true;storeDetails_=false;storeConfirm_=false;storeRow_=0;storeTab_=0;
            notice_=store_.Load()?"":"Catalogo ausente: configure aurorax-store.tsv.";
            RebuildStore();return "";
        }
        if(key==Favorite) {PreviewAuroraProfile();return "";}
        if(key==Guide) {
            homeDraft_=homeOrder_;approvedDraft_=approvedShortcuts_;
            homeEditSlot_=(focus_>=Apps && focus_<=System)?focus_-Apps:0;
            if(layout_==DashboardLayout::TopIcons)homeEditSlot_=ApprovedHome::GroupSlot(focus_)?ApprovedHome::GroupOrder(approvedShortcuts_)[ApprovedHome::GroupPosition(focus_)]*2:0;
            homeEditing_=true;notice_.clear();
            return "";
        }
        if(key==Accept) {
            if(layout_==DashboardLayout::TopIcons){OpenApprovedSlot();return "";}
            if(layout_&&focus_==8){storeActive_=true;storeDetails_=false;storeConfirm_=false;storeRow_=0;storeTab_=0;notice_=store_.Load()?"":"Catalogo ausente: configure aurorax-store.tsv.";RebuildStore();return "";}
            if(layout_&&focus_==9){section_=System;systemTab_=3;page_=Details;storeDownloads_=false;storeDownloadRow_=0;return "";}
            const int destination=(focus_>=Apps && focus_<=System)?
                Apps+homeOrder_[focus_-Apps]-'0':focus_;
            Enter(static_cast<Section>(destination));
        }
        return "";
    }
    if(GridListing()){
        if(gridFilter_){
            if(key==Left||key==Right){
                const int target=std::max(0,std::min(8,subtab_+(key==Right?1:-1)));
                if(target!=subtab_){subtab_=target;row_=0;Rebuild();}
                notice_.clear();return "";
            }
            if(key==Down||key==Accept||key==Back){gridFilter_=false;return "";}
            if(key==Up)return "";
            // Other existing shortcuts operate on the listing, not the filter row.
            gridFilter_=false;
        }
        if(key<=Down){
            if(section_==Library&&key==Up&&row_<GameGrid::Columns){gridFilter_=true;return "";}
            row_=GameGrid::Move(row_,static_cast<int>(Count()),key);
            notice_.clear();return "";
        }
    }
    if(page_==Listing && section_==Library && subtab_==8 && (key==PageDown || key==Search)) {
        if(Count())MovePin(key==PageDown?-1:1);
        return "";
    }
    if(page_==Listing && section_==Library && key==PageDown) {
        collectionPicker_=true;
        deleteCollectionConfirm_=false;
        pickerGame_=Count()?visible_[row_]->executable:"";
        collectionRow_=collectionIndex_;
        if(static_cast<size_t>(collectionRow_)>collections_.Items().size())
            collectionRow_=0;
        notice_.clear();
        return "";
    }
    if(page_==Listing && section_==Library && subtab_==5 && key==PageUp) {
        if(!collections_.Items().empty()) {
            collectionIndex_=(collectionIndex_+1)%
                             static_cast<int>(collections_.Items().size());
            row_=0;
            Rebuild();
        }
        notice_=collections_.Items().empty()?
            "Nenhuma colecao. RB abre o gerenciador.":
            "Colecao: "+collections_.Items()[collectionIndex_].name;
        return "";
    }
    if(page_==Listing && section_==Library && key==PageUp && Count()) {
        const std::string path=visible_[row_]->executable;
        std::vector<std::string>::iterator i=
            std::find(hidden_.begin(),hidden_.end(),path);
        const bool makeHidden=i==hidden_.end();
        if(makeHidden) hidden_.push_back(path);
        else hidden_.erase(i);
        const bool saved=SaveProfile();
        row_=0;
        Rebuild();
        notice_=makeHidden?"Jogo ocultado (aba Ocultos)":"Jogo reexibido";
        if(!saved) notice_+=" - falha ao salvar no dispositivo";
        return "";
    }
    if(page_==Listing && section_==Library && key==Guide) {
        searchDraft_=searchTerm_;
        searchCursor_=0;
        keyboardMode_=0;
        searchActive_=true;
        notice_.clear();
        return "";
    }
    if(page_==Listing && section_==Library && key==Search) {
        static const char* const sorts[]={"Nome A-Z","Nome Z-A","Dispositivo"};
        gameSort_=(gameSort_+1)%3;
        row_=0;
        Rebuild();
        notice_=std::string("Ordenar: ")+sorts[gameSort_];
        return "";
    }
    if(page_==Listing &&
       (section_==Apps || section_==Emulators || section_==Library) &&
       (key==Left || key==Right)) {
        const int last=section_==Emulators?2:section_==Library?8:3;
        if(key==Left && subtab_>0) --subtab_;
        if(key==Right && subtab_<last) ++subtab_;
        row_=0;
        notice_.clear();
        Rebuild();
        return "";
    }
    if(page_==Details) {
        if(key==PageUp && Count() &&
           (section_==Library || section_==Recent || section_==Favorites)) {
            const std::string path=visible_[row_]->executable;
            std::vector<std::string>::iterator found=std::find(pinned_.begin(),pinned_.end(),path);
            const bool adding=found==pinned_.end();
            if(adding && contains(hidden_,path)) {notice_="Reexiba o jogo antes de fixar.";return "";}
            if(adding && pinned_.size()>=PinLimit) {notice_="Limite de 32 jogos fixados.";return "";}
            if(adding) pinned_.push_back(path); else pinned_.erase(found);
            notice_=adding?"Jogo fixado no inicio.":"Jogo desafixado.";
            if(!SaveProfile()) notice_+=" Falha ao salvar; alteracao apenas nesta sessao.";
            if(section_==Favorites || (section_==Library && subtab_==8)) {
                page_=Listing;
                Rebuild();
            }
            return "";
        }
        if(key==Accept && section_!=Files && !visible_.empty()) {
            const std::string path=visible_[row_]->executable;
            if(section_==Apps||section_==Emulators){
                const size_t index=static_cast<size_t>(visible_[row_]-tools_.data());
                if(index<toolKinds_.size()&&toolKinds_[index]==2){
                    bool eligible=false;
                    for(const auto& item:PluginRegistry(PluginRegistry::DefaultRoots(dir_)).Scan())
                        if(item.executable==path&&PluginAllowed(item)){eligible=true;break;}
                    if(!eligible){notice_="Plugin alterado ou desativado. Y atualiza.";return "";}
                }
            }
            if(!isXexPath(path) || isContentPackage(*visible_[row_])) {
                if(isContentPackage(*visible_[row_])) {
                    const XboxPackageCheck check=ContentCatalog::CheckPackage(path);
                    notice_=ContentCatalog::PackageStatus(check.status);
#ifdef __XSDK__
                    if(!TransferRunning()&&check.status==XboxPackageCheck::StructurePresent){
                        XboxContentPackage package{};
                        if(ContentCatalog::ReadHeader(path,package)&&(package.contentType==0x7000u||package.contentType==0xd0000u))return path;
                    }
#endif
                } else notice_="Formato de executavel nao suportado.";
                return "";
            }
            if(TransferRunning()) {
                notice_="Conclua ou cancele a copia antes de iniciar outro executavel.";
                return "";
            }
#ifndef __XSDK__
            if(section_==Library || section_==Favorites || section_==Recent) {
                if(launched_.size()<4096 && !contains(launched_,path))
                    launched_.push_back(path);
                history_.erase(std::remove(history_.begin(),history_.end(),path),
                               history_.end());
                history_.insert(history_.begin(),path);
                if(history_.size()>HistoryLimit) history_.resize(HistoryLimit);
                if(!SaveProfile())
                    notice_="Historico desta sessao. Nao foi possivel salvar.";
            }
#endif
            return path;
        }
    } else {
        if(key==Up && row_>0) --row_;
        if(key==Down && size_t(row_+1)<Count()) ++row_;
    }
    if(!Count() || section_==System) return "";
    if(key==Favorite && (section_==Library || section_==Recent || section_==Favorites)) {
        const std::string path=visible_[row_]->executable;
        std::vector<std::string>::iterator found=std::find(favorites_.begin(),favorites_.end(),path);
        if(found==favorites_.end()) favorites_.push_back(path); else favorites_.erase(found);
        if(!SaveProfile()) notice_="Favorito desta sessão. Falha ao gravar no dispositivo.";
        if(section_==Favorites) page_=Listing;
        Rebuild();
    } else if(key==Accept && page_==Listing) {
        if(section_==Files && files_[row_].directory) {
            const std::string& here=folders_.back();
            const std::string child=FileWorkspace::Join(here,files_[row_].name);
            if(!fileWorkspace_.CanBrowse(child)) {notice_="Pasta indisponivel ou nao permitida.";return "";}
            folders_.push_back(child);row_=0;Reload();
        } else {
            page_=Details;
            RefreshTitleMetadata();
            // Viewing details alone must not change Recently Played.
        }
    }
    return "";
}

namespace {
const unsigned White=0xfff2f7f3u, Muted=0xffb7c9c1u, Lime=0xff9dec31u, Panel=0xff172c28u;
void panel(HomeCanvas& c,int x,int y,int w,int h,unsigned bg,bool active) {
    c.SeriesTile(x,y,w,h,active,220);
    if(bg!=Panel&&w>20&&h>20)c.Translucent(x+3,y+3,w-6,h-6,bg,34,8);
}
void buttonHint(HomeCanvas& c,int x,const char* key,const char* action,unsigned color) {
    (void)color;
    const int kind=key[0]=='A'?InfinityButtons::A:key[0]=='B'?InfinityButtons::B:key[0]=='X'?InfinityButtons::X:InfinityButtons::Y;
    InfinityButtons::Draw(c,kind,x,674,29);
    c.TextFit(x+38,672,c.Tr(action),19,White,150);
}
}

void ConsoleHome::DrawResidentPlugins(HomeCanvas& c) const {
    if(localPlugins_){DrawLocalPlugins(c);return;}
    c.Text(47,349,c.Tr("Plugins residentes / DashLaunch"),27,White,1170);
    if(residentSource_>=residentConfigs_.size()){
        c.Text(47,385,"Nenhum launch.ini legivel",18,Muted,1170);
    }else{
        const auto& config=residentConfigs_[residentSource_];
        c.Text(47,385,"Fonte "+number(residentSource_+1)+"/"+number(residentConfigs_.size())+" | "+config.path,18,Muted,1170);
        if(!config.valid)c.Text(51,430,"Configuracao invalida ou acima do limite de leitura.",20,White,1160);
        for(size_t i=0;i<config.slots.size();++i){
            const auto& slot=config.slots[i];const int y=415+static_cast<int>(i)*32;
            c.Text(51,y,"Slot "+number(i+1),18,White,110);
            c.Text(171,y,slot.path.empty()?"Vazio":slot.path,18,Muted,670);
            std::string state=slot.duplicate?"Ambiguo":slot.repeated?"Repetido":isLiveEmulationReference(slot.path)?"LiveEmulation":"";
#ifdef __XSDK__
            if(!slot.path.empty()&&!slot.duplicate&&!slot.repeated){
                const auto normalized=XsdkPaths::Normalize(slot.path);
                std::ifstream file(normalized.c_str(),std::ios::binary);char magic[4]{};
                const bool valid=!normalized.empty()&&file.read(magic,4)&&std::string(magic,4)=="XEX2";
                state=valid?(isLiveEmulationReference(slot.path)?"LE: XEX2":"XEX2 local"):"Ausente/invalido";
            }
#endif
            if(residentConflict(residentConfigs_,residentSource_,i))state+=(state.empty()?"":" | ")+std::string("Divergente");
            c.Text(850,y,state,17,Lime,375);
        }
    }
    c.Text(47,588,"Somente leitura: estado carregado e conexao nao verificados.",18,Muted,1170);
    InfinityButtons::Strip(c,47,610,"LB/RB Fonte | START Plugins locais | Y Atualizar | B Voltar",18,Lime,1170);
}
bool ConsoleHome::PluginAllowed(const PluginDescriptor& item)const{
    return item.manifestEnabled&&item.headerValid&&!item.conflict&&!disabledPluginIds_.count(PluginRegistry::Key(item.id));
}
void ConsoleHome::RefreshLocalPlugins(){
    localPluginItems_=PluginRegistry(PluginRegistry::DefaultRoots(dir_)).Inventory();
    clampRow(localPluginRow_,localPluginItems_.size());
}
void ConsoleHome::ToggleLocalPlugin(){
    if(localPluginRow_<0||static_cast<size_t>(localPluginRow_)>=localPluginItems_.size())return;
    const auto& item=localPluginItems_[localPluginRow_];
    const auto key=PluginRegistry::Key(item.id);
    if(key.empty()||!item.manifestEnabled||!item.headerValid||item.conflict){notice_="Plugin indisponivel; manifesto, XEX ou ID conflitante.";return;}
    auto old=disabledPluginIds_;
    const bool enable=disabledPluginIds_.count(key)!=0;
    if(enable)disabledPluginIds_.erase(key);
    else if(disabledPluginIds_.size()<256)disabledPluginIds_.insert(key);
    else{notice_="Limite de preferencias de plugins.";return;}
    if(!SaveProfile()){disabledPluginIds_=old;notice_="Falha ao salvar preferencia.";return;}
    notice_=enable?"Atalho local habilitado.":"Atalho local desabilitado.";
    RecordEvent(enable?"Plugin local habilitado":"Plugin local desabilitado");
}
void ConsoleHome::DrawLocalPlugins(HomeCanvas& c)const{
    c.Text(47,349,"Plugins locais | "+number(localPluginItems_.size()),27,White,1170);
    if(localPluginItems_.empty())c.Text(48,422,"Nenhum manifesto plugin.axplugin reconhecido.",21,Muted,1170);
    const int first=(localPluginRow_/4)*4;
    for(int i=first;i<first+4&&static_cast<size_t>(i)<localPluginItems_.size();++i){
        const auto& item=localPluginItems_[i];const int y=390+(i-first)*47;
        panel(c,48,y,1184,43,Panel,i==localPluginRow_);
        c.Text(62,y+2,item.name+" | "+item.version,19,White,690);
        c.Text(64,y+23,item.id+" | "+item.category,14,Muted,690);
        const char* state=item.conflict?"ID conflitante":!item.headerValid?"XEX ausente/invalido":!item.manifestEnabled?"Manifesto desativado":PluginAllowed(item)?"Habilitado":"Desabilitado localmente";
        c.Text(806,y+9,state,19,PluginAllowed(item)?Lime:Muted,400);
    }
    c.Text(48,585,"Preferencia local; nao altera launch.ini, nao carrega modulos residentes.",18,Muted,1170);
    InfinityButtons::Strip(c,48,610,"X Alternar | START DashLaunch | Y Atualizar | B Voltar",18,Lime,1170);
}
void ConsoleHome::DrawHomeEditor(HomeCanvas& c) const {
    if(layout_==DashboardLayout::TopIcons){
        const char* names[]={"Instalador","Gerenciador","System Link","IPTV","Loja Infinity","Conquistas"};
        c.TextFit(45,85,"Personalizar início",38,White,1180);
        c.TextFit(45,143,"Organize a prioridade dos atalhos dentro dos grupos",22,Muted,1180);
        for(int i=0;i<6;++i){int x=45+(i%3)*400,y=200+(i/3)*155;int item=approvedDraft_[i]-'0';c.SeriesTile(x,y,385,140,i==homeEditSlot_);InfinityIcons::Draw(c,13+item,x+25,y+23,42,White);c.TextFit(x+25,y+77,names[item],25,White,330);}
        InfinityButtons::Strip(c,45,548,"Direcional Selecionar | LB/RB Mover | X Restaurar | Y Salvar | B Cancelar",18,Lime,1180);return;
    }
    c.Text(44,82,c.Tr("Personalizar inicio"),40,White,1190);
    c.Text(46,150,c.Tr("Organize os quatro atalhos da segunda fileira."),24,Muted,1180);
    c.Text(46,196,"Biblioteca e Cloud360 IPTV permanecem no topo.",21,Muted,1180);
    for(int i=0;i<4;++i) {
        const int x=42+i*302, item=homeDraft_[i]-'0';
        panel(c,x,282,289,168,Panel,i==homeEditSlot_);
        InfinityIcons::Draw(c, (item==0?2:item==1?1:item==2?14:10),x+112,303,60,White);
        c.Text(x+16,374,c.Tr(labels[Apps+item]),26,i==homeEditSlot_?Lime:White,258);
        c.Text(x+16,414,"Posicao "+number(i+1),18,Muted,252);
    }
    c.Text(46,500,"Direcional: selecionar | LB/RB: mover o atalho",23,White,1180);
    InfinityButtons::Strip(c,46,550,"X Restaurar padrao | Y Salvar | B Cancelar",23,Lime,1180);
}
void ConsoleHome::DrawAuroraProfile(HomeCanvas& c) const {
    c.Text(44,78,"Importar perfil Aurora",40,White,1180);
    c.Text(48,152,"Previa da mesclagem com jogos encontrados neste console",23,Muted,1180);
    c.Box(44,215,1190,272,Panel,12);
    c.Text(70,244,"Novos favoritos: "+number(importFavorites_.size()),30,Lime,1130);
    c.Text(70,308,"Registros de historico encontrados: "+number(importHistory_.size()),26,White,1130);
    c.Text(70,370,"Recentes locais tem prioridade. Jogos ambiguos ou ausentes sao ignorados.",21,Muted,1130);
    c.Text(70,417,"Nenhum jogo ou banco Aurora sera alterado.",22,Muted,1130);
    InfinityButtons::Strip(c,48,560,"A Confirmar mesclagem | B Cancelar",24,Lime,1180);
}
void ConsoleHome::DrawStoreManage(HomeCanvas& c,const StoreItem& item) const{
    c.Text(55,250,item.name+" | Gerenciar",28,White,1150);
    const char* actions[]={"Desinstalar (arquivar pasta e dados)","Restaurar ultimo backup","Arquivar pacote baixado"};
    const bool ready[]={store_.InstalledFolder(item),store_.BackupAvailable(item),store_.PackagePresent(item)};
    for(int i=0;i<3;++i){
        const int y=305+i*69;
        panel(c,55,y,1158,58,Panel,i==storeManageRow_);
        const int icons[]={40,39,35};InfinityIcons::Draw(c,icons[i],70,y+12,30,ready[i]?White:Muted);
        c.TextFit(115,y+9,actions[i],22,ready[i]?White:Muted,790);
        c.Text(935,y+9,ready[i]?"Disponivel":"Indisponivel",18,ready[i]?Lime:Muted,265);
    }
    c.Text(55,535,"Backups no destino Store | nenhum arquivo apagado",19,Muted,1150);
    InfinityButtons::Strip(c,55,583,storeManageConfirm_?"A Confirmar | B Cancelar":"Cima/Baixo Selecionar | A Preparar | B Voltar",20,Lime,1150);
}
std::string ConsoleHome::StoreSourceLabel()const{
    for(const auto& row:StoreSourceDirectory::Rows(store_.Providers()))if(row.id==storeSource_)return row.name;
    return storeSource_;
}
void ConsoleHome::DrawProviders(HomeCanvas& c) const{
    c.Text(44,78,"INFINITY Store | Central de fontes",37,White,1175);
    c.Text(48,136,"Abertura por fonte | HTTPS autorizado | SHA-256",20,Muted,1175);
    const auto rows=StoreSourceDirectory::Rows(store_.Providers());
    const int first=providerRow_/6*6;
    for(int i=first;i<first+6&&static_cast<size_t>(i)<rows.size();++i){
        const auto& p=rows[i];const int y=182+(i-first)*65;
        panel(c,45,y,1189,58,Panel,i==providerRow_);
        InfinityIcons::Draw(c,48,62,y+12,31,White);
        c.TextFit(107,y+3,p.name,23,White,750);
        c.TextFit(107,y+30,p.note,17,Muted,800);
        c.Text(950,y+12,p.id==storeSource_?"Selecionado":p.provider>=0?"Configurado":p.id=="@unity"?"Integrado":p.id.empty()||p.id=="*"?"Local":"Pendente",17,p.id==storeSource_?Lime:Muted,270);
    }
    if(!store_.ProvidersValid())c.Text(45,575,"Configuracao externa invalida; sem sincronizacao.",17,Muted,1170);
    InfinityButtons::Strip(c,45,602,"A Abrir fonte | START Sincronizar | X Todos | Y Recarregar | B Voltar",18,Lime,1175);
}
void ConsoleHome::DrawStore(HomeCanvas& c) const {
    if(storeProviders_){DrawProviders(c);return;}
    if(!storeManage_){DrawModernStore(c);return;}
    InfinityIcons::Draw(c,4,46,91,32,White);c.TextFit(91,78,"INFINITY Store",40,White,1128);
    c.Text(48,133,StoreSourceLabel()+" | Destino: "+StoreStorage::Label(store_.Storage()),21,Muted,1175);
    const char* const tabs[]={"Todos","Jogos","XBLA","Homebrew","Apps","Emuladores","DLC","PT-BR"};
    for(int i=0;i<8;++i) {
        const int x=45+i*149;
        panel(c,x,177,140,44,Panel,i==storeTab_);
        c.Text(x+8,185,c.Tr(tabs[i]),18,i==storeTab_?Lime:White,124);
    }
    if(storeVisible_.empty()) {
        c.Text(48,270,"Nenhum item no catalogo desta categoria.",24,White,1175);
        c.Text(48,320,"Configure aurorax-store.tsv com fontes e licencas autorizadas.",20,Muted,1175);
    } else if(storeDetails_) {
        const StoreItem& item=store_.Items()[storeVisible_[storeRow_]];
        if(storeManage_){DrawStoreManage(c,item);return;}
        c.Text(55,260,item.name,32,White,1150);
        c.Text(55,315,"Versao: "+item.version+" | "+StorageInfo::FormatBytes(item.bytes),24,Muted,1150);
        c.Text(55,367,"Licenca / autorizacao declarada: "+item.rights,22,Muted,1150);
        c.Text(55,415,store_.Installed(item)?"Instalado (presenca local)":store_.InstalledFolder(item)?"Outra versao local | R3 atualiza com backup":store_.Progress(item).state==StoreProgress::NativePaused?"Parcial salvo | START retoma | RB duas vezes descarta":store_.Progress(item).state==StoreProgress::NativeVerified?"Verificado no Xbox (registro local)":store_.Staged(item)?"Arquivo local encontrado (tamanho)":
            store_.NativeQueued(item)?"Na fila nativa":store_.Requested(item)?"Solicitado na fila PC":"Disponivel no catalogo",22,Lime,1150);
        if(item.category=="translation") {
            char ids[128];std::sprintf(ids,"Title %08X | Media %08X | XEX %08X",
                                       item.titleId,item.mediaId,item.gameVersion);
            c.Text(55,467,ids,20,Muted,1150);
            c.Text(55,505,storeTranslationMatch_?"Cabecalho local coincide; TU ativa nao validada":
                "Nenhum XEX local correspondente; pedido bloqueado",20,storeTranslationMatch_?Lime:Muted,1150);
        } else c.Text(55,470,item.provider.empty()?item.url:"Provedor: "+item.provider+" | "+item.url,18,Muted,1150);
#if defined(INFINITY_XSDK_HTTPS)
        c.Text(55,560,store_.Staged(item)&&!store_.Installed(item)&&(item.category=="app"||item.category=="homebrew"||item.category=="emulator")?"R3 duas vezes: instalar/atualizar XEX/ZIP | RB Gerenciar":store_.NativeQueued(item)?"LB Remover da fila nativa":"LB Adicionar a fila nativa",20,Lime,1150);
#endif
        c.Text(55,535,"RB Gerenciar (ou descartar parcial) | LB Fila nativa",18,Muted,1150);
        InfinityButtons::Strip(c,55,590,storeConfirm_?"A Confirmar pedido | B Cancelar":"Start Baixar no Xbox | A Pedido PC | X Remover PC | B Voltar",21,Lime,1150);
        return;
    } else {
        const int first=(storeRow_/5)*5;
        for(int i=first;i<first+5&&static_cast<size_t>(i)<storeVisible_.size();++i) {
            const StoreItem& item=store_.Items()[storeVisible_[i]];
            const int y=247+(i-first)*64;
            panel(c,45,y,1189,56,Panel,i==storeRow_);
            c.Text(60,y+5,item.name,24,White,755);
            c.Text(60,y+32,item.version+" | "+StorageInfo::FormatBytes(item.bytes),17,Muted,750);
            c.Text(835,y+15,store_.Staged(item)?"Arquivo local":store_.Requested(item)?"Na fila PC":"Disponivel",20,Lime,360);
        }
    }
    InfinityButtons::Strip(c,48,590,"A Detalhes | LB Destino HDD/USB | R3 Provedores | Y Atualizar | B Voltar",21,Lime,1175);
}
void ConsoleHome::DrawHome(HomeCanvas& c) const {
    if(weatherPanel_){DrawWeatherPanel(c);return;}
    if(storeActive_) {DrawStore(c);return;}
    if(importPreview_) {DrawAuroraProfile(c);return;}
    if(homeEditing_) {DrawHomeEditor(c);return;}
    if(layout_){DrawModernHome(c,layout_);return;}
    c.Text(44,76,c.Tr("Início"),42,White,330);
    c.Text(459,86,c.Tr("R3 Guia | LB Notificacoes | RB Loja | Start Personalizar"),18,Muted,775);
    panel(c,42,140,672,196,0xff042417u,focus_==Library);
    panel(c,728,140,510,196,0xff003e33u,focus_==Iptv);
    c.Image(0,0,342,222,436,150,260,173);
    c.Image(344,0,254,222,1035,155,183,163);
    InfinityIcons::Draw(c,23,70,160,43,Lime);
    c.Text(69,214,c.Tr("Biblioteca"),43,White,350);
    c.Text(70,278,c.Tr("Jogos instalados"),23,Muted,350);
    c.Text(752,219,"Cloud360 IPTV",35,White,283);
    c.Text(753,275,c.Tr("TV ao vivo, filmes e séries"),20,Muted,280);
    for(int i=0;i<4;++i) {
        int x=42+i*302;
        panel(c,x,357,289,75,Panel,focus_==Apps+i);
        const int item=homeOrder_[i]-'0';
        InfinityIcons::Draw(c,item==0?2:item==1?1:item==2?14:10,x+24,376,38,White);
        c.Text(x+80,375,c.Tr(labels[Apps+item]),23,White,205);
    }
    c.Box(767,456,1,174,0xff355148u);
    for(int side=0;side<2;++side) {
        int x=side?789:42, w=side?449:701;
        bool focused=focus_==(side?Favorites:Recent);
        if(focused) c.Box(x,490,w,3,Lime);
        c.Text(x,448,c.Tr(labels[side?Favorites:Recent]),side?24:28,focused?Lime:White,w);
        const std::vector<std::string> paths=side?HomeShortcuts():history_;
        int shown=0, cardWidth=(w-18)/3;
        for(size_t i=0;i<paths.size() && shown<3;++i) {
            const Game* g=Installed(paths[i]); if(!g || contains(hidden_,paths[i])) continue;
            int left=x+shown*(cardWidth+9);
            c.Box(left,504,cardWidth,126,Panel,8);
            const std::string backFile=ArtPath(*g,false), coverFile=ArtPath(*g,true);
            c.Box(left+1,505,cardWidth-2,84,0xff194739u,6);
            const bool backdrop=appearance_.ShowBackdrop() && !backFile.empty() && c.LocalArt(backFile,false,left+1,505,cardWidth-2,84);
            const bool cover=appearance_.ShowCover() && !coverFile.empty() && c.LocalArt(coverFile,true,left+(cardWidth-56)/2,505,56,84);
            if(!backdrop && !cover) {
                int sx=600,sy=66;
                if(g->name=="Halo 3") sx=742;
                else if(g->name=="Gears of War 3") sy=132;
                if(appearance_.ShowBackdrop() && (g->name=="Forza Horizon" || g->name=="Halo 3" || g->name=="Gears of War 3"))
                    c.Image(sx,sy,140,64,left+1,505,cardWidth-2,84);
                else InfinityIcons::Draw(c,1,left+cardWidth/2-20,525,40,Muted);
            }
            c.Text(left+8,594,g->name,16,White,cardWidth-16);
            if(side && contains(pinned_,paths[i])) c.Text(left+8,508,c.Tr("FIXADO"),14,Lime,cardWidth-16);
            ++shown;
        }
        if(!shown) {
            c.Box(x,503,w,124,Panel,9);
            c.Text(x+15,543,side?c.Tr("LB nos detalhes fixa um jogo."):c.Tr("Os jogos abertos aparecem aqui."),19,Muted,w-26);
        }
    }
}

namespace {
std::string hexId(unsigned value) {
    char text[9];
    std::sprintf(text,"%08X",value);
    return text;
}
}
void ConsoleHome::DrawTrainers(HomeCanvas& c) const {
    c.Text(48,241,number(trainers_.size())+" trainers locais | Versao do cabecalho, nao TU ativo",20,Muted,1175);
    if(trainers_.empty()) {
        c.Text(48,305,"Nenhum trainer registrado em Trainers/"+hexId(contentInfo_.titleId),24,White,1170);
        c.Text(48,351,"Adicione um utilitario XEX e seu manifesto trainer.axtrainer.",20,Muted,1170);
    }
    const int first=(contentRow_/4)*4;
    for(int i=first;i<first+4 && static_cast<size_t>(i)<trainers_.size();++i) {
        const TrainerEntry& item=trainers_[i];
        const int y=280+(i-first)*66;
        panel(c,45,y,1189,59,Panel,i==contentRow_);
        InfinityIcons::Draw(c,41,63,y+13,29,White);
        c.TextFit(105,y+4,item.name,23,White,665);
        c.TextFit(105,y+31,"Media "+hexId(item.mediaId)+" | Versao "+hexId(item.version),17,Muted,665);
        const TrainerEntry::Match match=item.Compatibility(contentInfo_);
        const char* status=match==TrainerEntry::HeaderMatches?"Cabecalho coincide":
            match==TrainerEntry::DifferentMedia?"Media ID diferente":
            match==TrainerEntry::DifferentVersion?"Versao diferente":"Indeterminado";
        const TrainerEntry::TuCheck tu=item.CheckUpdates(contentItems_);
        const char* tuText=tu==TrainerEntry::LegacyHeaderOnly?"TU nao declarada":
            tu==TrainerEntry::ActiveUnknown?"Sem TU: estado desconhecido":
            tu==TrainerEntry::RequiredLocal?"TU localizada, ativa?":
            tu==TrainerEntry::RequiredMissing?"TU exigida ausente":
            tu==TrainerEntry::RequiredAmbiguous?"TUs ambiguas":"TU sem numero confirmado";
        c.Text(810,y+3,status,18,match==TrainerEntry::HeaderMatches?Lime:Muted,398);
        c.Text(810,y+30,tuText,17,Muted,398);
    }
    c.Text(48,555,"Use apenas offline e com arquivos confiaveis. TU ativo e funcionamento nao verificados.",19,Muted,1180);
    c.Text(48,600,trainerConfirm_?"A Confirmar saida para trainer | B Cancelar":
        "A Abrir utilitario independente | Y Atualizar | B Voltar",18,Lime,1175);
}
bool ConsoleHome::TakeArtRequest(unsigned& title,bool& cover,bool& remote){
    if(!artPending_)return false;artPending_=false;title=artTitle_;cover=artCover_;remote=artRemote_;return true;
}
void ConsoleHome::RefreshDashboardUpdate(){
    updateConfirm_=false;updateKnown_=DashboardUpdate::Read(dir_,updateItem_);
    notice_=updateKnown_?"Manifesto local lido; confira SHA-256 da Release.":"Copie infinity360-update.tsv da Release para MenuPreview.";
}
bool ConsoleHome::TakeDashboardUpdate(StoreItem& item){
    if(!updatePending_)return false;updatePending_=false;item=updateItem_;return true;
}
void ConsoleHome::DashboardUpdateResult(const std::string& message,bool success){
    notice_=message;RecordEvent(success?"Dashboard candidata instalada":"Atualizacao recusada");
}
void ConsoleHome::DrawDashboardUpdate(HomeCanvas& c) const{
    c.Text(44,78,"INFINITY | Atualizar dashboard",38,White,1170);
    c.Text(48,142,"Instalacao lado a lado. Nao altera a versao em execucao nem launch.ini.",21,Muted,1170);
    if(updateKnown_){
        c.Text(48,205,updateItem_.name+" | "+StorageInfo::FormatBytes(updateItem_.bytes),30,White,1170);
        c.Text(48,255,"SHA-256: "+updateItem_.sha256,18,Muted,1170);
        c.Text(48,310,"Origem: UserFiles/Store/"+updateItem_.id+".pkg",20,White,1170);
        c.Text(48,355,"Destino: UserFiles/DashboardVersions/"+updateItem_.id,20,White,1170);
        c.Text(48,405,store_.InstalledFolder(updateItem_)?"Pasta candidata presente. Inicie default.xex externamente.":"Pacote ainda nao instalado neste destino.",22,Lime,1170);
    }else c.Text(48,250,"Nenhum manifesto valido. Consulte a Release no GitHub.",24,White,1170);
    c.Text(48,490,"Requer escrita, instalador ZIP e enable-dashboard-update.txt.",20,Muted,1170);
    c.Text(48,535,"Versao anterior preservada; retorno manual pelo gerenciador externo.",20,Muted,1170);
    InfinityButtons::Strip(c,48,600,updateConfirm_?"A Confirmar | B Cancelar":"A Instalar candidata | Y Consultar | B Voltar",19,Lime,1170);
}
void ConsoleHome::PrepareCoverBatch(){
    coverBatch_.clear();std::set<unsigned> seen;
    // Limit each manually initiated batch to 32 missing titles; skip hidden titles and existing artwork.
    for(const auto& game:games_){
        if(contains(hidden_,game.executable))continue;
        const auto info=ContentCatalog::DescribeTitle(game.executable,isContentPackage(game));
        if(!info.known||!info.titleId||!seen.insert(info.titleId).second)continue;
        const auto personal=FileWorkspace::Join(ArtImport::Folder(dir_,info.titleId),"cover.rgba");
        const auto legacy=artFile(ArtFolder(game),true,'0');
        if(fileExists(personal)||fileExists(legacy))continue;
        coverBatch_.push_back(info.titleId);
        if(coverBatch_.size()>=32)break;
    }
}
bool ConsoleHome::TakeCoverBatch(std::vector<unsigned>& titles){
    if(!coverBatchPending_)return false;coverBatchPending_=false;titles.swap(coverBatch_);return true;
}
void ConsoleHome::CoverBatchResult(unsigned saved,unsigned failed,bool cancelled){
    artDirty_=true;notice_="Capas: "+number(saved)+" salvas, "+number(failed)+" indisponiveis"+(cancelled?" | cancelado.":".");
    RecordEvent("Lote de capas concluido");
}
void ConsoleHome::ArtResult(const std::string& message,bool success){
    artDirty_=true;notice_=message;RecordEvent(success?"Arte importada":"Importacao de arte recusada");
}
bool ConsoleHome::TakeBackupRequest(XboxContentPackage& selected){
    if(!backupPending_)return false;backupPending_=false;selected=backupItem_;return true;
}
void ConsoleHome::BackupResult(const std::string& message,bool success){
    if(success){if(saveCenter_)RefreshSaveRecords();else backups_=ContentBackup::List(fileWorkspace_,dir_,contentInfo_.titleId);}
    notice_=message;RecordEvent(success?"Backup de conteudo verificado":"Backup recusado");
}
bool ConsoleHome::TakeRecoverRequest(ContentBackup::Record& selected,bool& verifyOnly){
    if(!recoverPending_)return false;recoverPending_=false;selected=recoverItem_;verifyOnly=verifyOnly_;return true;
}
void ConsoleHome::RecoverResult(const std::string& message,bool success){
    if(success){if(saveCenter_)RefreshSaveRecords();else stagedItems_=ContentBackup::Staged(fileWorkspace_,dir_,contentInfo_.titleId);}
    notice_=message;RecordEvent(verifyOnly_?(success?"Backup de conteudo verificado":"Verificacao de backup recusada"):(success?"Copia de conteudo recuperada":"Recuperacao recusada"));
}
bool ConsoleHome::TakeUnityRequest(XboxExecutionInfo& info){
    if(!unityPending_)return false;
    unityPending_=false;info=contentInfo_;return true;
}
bool ConsoleHome::TakeUnityDownload(XboxExecutionInfo& info,UnityUpdate& update){
    if(!unityDownloadPending_)return false;
    unityDownloadPending_=false;info=contentInfo_;update=unityDownload_;return true;
}
void ConsoleHome::UnityDownloadResult(const std::string& message,bool success){
    if(success)stagedItems_=ContentBackup::Staged(fileWorkspace_,dir_,contentInfo_.titleId);
    notice_=message;RecordEvent(success?"TU salva em staging":"Download TU recusado");
}
void ConsoleHome::UnityResult(const std::vector<UnityUpdate>& updates,const std::string& message,bool success){
    if(success){unityUpdates_=updates;unityRow_=0;}else{unityUpdates_.clear();unityRow_=0;}
    unityConfirm_=false;
    notice_=message;RecordEvent(success?"XboxUnity TU consultado":"Consulta XboxUnity falhou");
}
void ConsoleHome::DrawUnity(HomeCanvas& c) const{
    c.Text(44,78,"XboxUnity | Title Updates",37,White,1170);
    c.Text(48,136,"Title "+hexId(contentInfo_.titleId)+" | Base "+hexId(contentInfo_.baseVersion)+" | Media "+hexId(contentInfo_.mediaId),21,Muted,1170);
    c.Text(48,180,"Consulta online: compatibilidade Media ID e ativacao ainda nao verificadas.",19,Muted,1170);
    if(unityUpdates_.empty())c.Text(48,285,"Nenhum resultado disponivel. Y consulta novamente.",24,White,1170);
    const int first=(unityRow_/5)*5;
    for(int i=first;i<first+5&&static_cast<size_t>(i)<unityUpdates_.size();++i){
        const auto& tu=unityUpdates_[i];const int y=235+(i-first)*65;
        panel(c,45,y,1189,58,Panel,i==unityRow_);
        c.Text(60,y+6,"TU ID "+number(tu.id)+" | "+(tu.version.empty()?"Versao nao informada":tu.version),23,White,840);
        c.Text(60,y+33,tu.filename.empty()?"Nome original indisponivel":tu.filename,16,Muted,840);
        c.Text(905,y+12,tu.bytes?StorageInfo::FormatBytes(tu.bytes):"Tamanho desconhecido",18,Muted,300);
    }
    InfinityButtons::Strip(c,48,590,unityConfirm_?"A Confirmar download | B Cancelar":"A Baixar TU verificada | Y Consultar | B Voltar | Sem ativacao",19,Lime,1175);
}
void ConsoleHome::RefreshContentInstall(){
    contentInstallConfirm_=false;std::string error;
    contentInstallKnown_=!contentInstallBackup_.empty()?ContentInstall::ReadBackup(fileWorkspace_,dir_,contentInfo_,contentInstallBackup_,contentInstallItem_,error):contentInstallStaged_.empty()?ContentInstall::Read(fileWorkspace_,dir_,contentInfo_,contentInstallItem_,error):ContentInstall::ReadStaged(fileWorkspace_,dir_,contentInfo_,contentInstallStaged_,contentInstallItem_,error);
    notice_=contentInstallKnown_?"Pacote local identificado. Confira origem e SHA-256.":error;
}
bool ConsoleHome::TakeContentInstall(ContentInstall::Request& item,XboxExecutionInfo& game){
    if(!contentInstallPending_)return false;contentInstallPending_=false;
    XboxExecutionInfo current{};
    if(!Count()||!ContentCatalog::ReadExecutionInfo(visible_[row_]->executable,current)||
       current.titleId!=contentInfo_.titleId||current.mediaId!=contentInfo_.mediaId||current.baseVersion!=contentInfo_.baseVersion){
        notice_="Jogo alterado; selecione novamente.";return false;
    }
    item=contentInstallItem_;game=current;return true;
}
void ConsoleHome::ContentInstallResult(const std::string& message,bool success){
    if(success)contentItems_=ContentCatalog(ContentCatalog::DefaultRoots(dir_)).ScanExtras(contentInfo_.titleId);
    notice_=message;RecordEvent(success?"Conteudo copiado e verificado":"Instalacao de conteudo recusada");
}
void ConsoleHome::DrawContentInstall(HomeCanvas& c) const{
    c.Text(44,78,"Instalar TU / DLC autorizada",36,White,1170);
    c.Text(48,135,"Title "+hexId(contentInfo_.titleId)+" | Media "+hexId(contentInfo_.mediaId),21,Muted,1170);
    if(contentInstallKnown_){
        const auto& i=contentInstallItem_;
        c.Text(48,205,!contentInstallBackup_.empty()?"Reinstalar backup TU / DLC":i.package.contentType==2?"DLC local":ContentInstall::LegacyFilename(i.filename)?"Title Update legado":"Title Update moderno",28,White,1170);
        c.Text(48,251,i.filename+" | "+StorageInfo::FormatBytes(i.bytes),22,White,1170);
        c.Text(48,295,"SHA-256 "+i.sha256,18,Muted,1170);
        c.Text(48,350,i.package.contentType==0xb0000u&&ContentInstall::LegacyFilename(i.filename)?"HDD/Cache/"+i.filename:"HDD/Content/0000000000000000/"+hexId(i.package.titleId)+"/"+hexId(i.package.contentType),19,Lime,1170);
    }else c.Text(48,215,!contentInstallBackup_.empty()?"Backup nao elegivel para instalacao; use copia pessoal.":contentInstallStaged_.empty()?"Manifesto infinity360-content-import.tsv e pacote original em UserFiles/ContentImport.":"TU sem recibo valido. Use importacao manual (R3) com nome original.",20,White,1170);
    c.Text(48,425,"Requer enable-content-install.txt e enable-file-writes.txt.",20,Muted,1170);
    c.Text(48,470,"Somente copia verificada. Nao altera licencas, nao remove outras TUs.",20,Muted,1170);
    c.Text(48,515,"Ativacao e reconhecimento pelo Xbox permanecem nao confirmados.",20,Muted,1170);
    InfinityButtons::Strip(c,48,600,contentInstallConfirm_?"A Confirmar | B Cancelar":"A Instalar | Y Consultar | B Voltar",19,Lime,1170);
}
void ConsoleHome::DrawContent(HomeCanvas& c) const {
    if(contentInstallActive_){DrawContentInstall(c);return;}
    if(unityActive_){DrawUnity(c);return;}
    InfinityIcons::Draw(c,35,46,91,32,White);c.TextFit(91,78,c.Tr("Conteudo instalado"),40,White,1140);
    if(!contentKnown_) {
        c.Text(48,150,"Title ID indisponivel para este executavel.",26,White,1170);
        c.Text(48,198,"Nao e possivel associar TU, DLC ou saves com seguranca.",21,Muted,1170);
        c.Text(48,260,"B Voltar | Y Atualizar",22,Lime,1170);
        return;
    }
    c.Text(45,135,"Title ID: "+hexId(contentInfo_.titleId)+"    Media ID: "+
           hexId(contentInfo_.mediaId)+"    Versao: "+hexId(contentInfo_.version),
           21,Muted,1180);
    const char* const tabs[]={"Title Updates","DLC","Saves","Trainers","TU Baixadas","Backups","TU Arquivadas","TU Retiradas"};
    for(int i=0;i<8;++i) {
        const int x=45+i*149;
        panel(c,x,183,141,47,Panel,i==contentTab_);
        const int icons[]={36,37,38,41,5,39,40,55};
        InfinityIcons::Draw(c,icons[i],x+7,194,21,i==contentTab_?Lime:White);
        c.TextFit(x+34,191,c.Tr(tabs[i]),16,White,101);
    }
    if(contentTab_==3) {DrawTrainers(c);return;}
    c.Text(48,241,number(ContentCount())+(contentTab_==0?" TUs locais | Ativacao desconhecida":" pacotes encontrados")+((contentTab_==0||contentTab_==1||contentTab_==4)&&!detailMetadata_.package?" | R3 Instalar TU/DLC local":" | Inventario"),20,Muted,1175);
    if(ContentCount()==0)c.Text(48,305,"Nenhum pacote encontrado neste local.",24,White,1170);
    const int first=(contentRow_/5)*5;
    for(int i=first;i<first+5;++i) {
        const XboxContentPackage* item=ContentAt(i);
        if(!item)break;
        const int y=280+(i-first)*62;
        panel(c,45,y,1189,55,Panel,i==contentRow_);
        c.Text(62,y+6,item->name,23,White,700);
        std::string status;
        if(contentTab_==0||contentTab_==4||contentTab_==6||contentTab_==7) {
            if(contentInfo_.mediaId&&item->mediaId&&item->mediaId!=contentInfo_.mediaId)status="Media ID diferente";
            else if(contentInfo_.baseVersion&&item->baseVersion&&item->baseVersion!=contentInfo_.baseVersion)status="Base Version diferente";
            else if(!contentInfo_.mediaId||!item->mediaId||!contentInfo_.baseVersion||!item->baseVersion)status="Compatibilidade inconclusiva";
            else status="Identidade compativel";
            c.Text(62,y+31,"Versao: "+hexId(item->version)+" | Base: "+hexId(item->baseVersion)+(contentTab_==0?(item->legacyCache?" | CACHE LEGADO":" | CONTENT"):contentTab_==7?" | RETIRADA":" | NAO ATIVADA"),17,Muted,690);
        } else if(contentTab_==5)status="Backup | "+StorageInfo::FormatBytes(backups_[i].bytes);
        else if(contentTab_==1)status="Marketplace / DLC";
        else {status="Save | "+item->saveDevice;
            const auto& owner=item->saveOwner;
            c.Text(62,y+31,owner.empty()?"Perfil nao identificado":owner=="0000000000000000"?"Pasta compartilhada":"Perfil ..."+owner.substr(owner.size()>6?owner.size()-6:0),17,Muted,690);
        }
        c.Text(780,y+14,status,20,Muted,430);
    }
    c.Text(48,601,recoverConfirm_||archiveTuConfirm_||restoreArchivedConfirm_||backupConfirm_||toggleTuConfirm_?"A Confirmar | B Cancelar":contentTab_==7?"A Recolocar TU | RB Backup | Y Atualizar | B Voltar":contentTab_==6?"A Recuperar TU | RB Backup | Y Atualizar | B Voltar":contentTab_==5?"A Copia | X Verificar SHA-256 | R3 Reinstalar TU/DLC | Y Atualizar | B Voltar":contentTab_==4?"A Instalar | X Arquivar TU | RB Backup | Y Atualizar | B Voltar":contentTab_==0?"A Caminho | X Retirar TU | START XboxUnity | RB Backup | Y Atualizar":"A Caminho | RB Backup | Y Atualizar | B Voltar",18,Lime,1175);
}

void ConsoleHome::RefreshDiscInstall(){
    discConfirm_=false;
#if defined(INFINITY_DISC_INSTALL)
    static const char* drives[]={"hdd:/","usb0:/","usb1:/","usb2:/"};
    discIdentity_={};
    discKnown_=ContentCatalog::ReadExecutionInfo("dvd:/default.xex",discIdentity_);
    discVolume_=StorageInfo::Read(drives[discDestination_]);
#else
    discKnown_=false;discVolume_={};
#endif
}
bool ConsoleHome::TakeDiscInstallRequest(DiscInstallRequest& request){
    if(!discPending_)return false;
    discPending_=false;
    request=pendingDisc_;return true;
}
bool ConsoleHome::TakePackageImport(PackageLaunch::Ticket& ticket){
    if(!packageImportPending_)return false;packageImportPending_=false;ticket=packageImportTicket_;return true;
}
void ConsoleHome::PackageImportResult(const std::string& message,bool success){
    if(success){ScanLibrary();Rebuild();}notice_=message;RecordEvent(success?"Pacote GOD/XBLA importado":"Importacao recusada");
}
bool ConsoleHome::TakeDiscSystemExit(){
    const bool requested=discSystemExit_;discSystemExit_=false;return requested;
}
void ConsoleHome::DiscInstallResult(const std::string& message,bool success){
    if(success){ScanLibrary();Rebuild();}
    RefreshDiscInstall();notice_=message;RecordEvent(success?"DVD copiado e verificado":"Instalacao DVD interrompida");
}
void ConsoleHome::DrawDiscInstall(HomeCanvas& c) const{
    c.Text(45,349,"Instalar a partir do DVD",28,White,1170);
#if defined(INFINITY_DISC_INSTALL)
    static const char* modes[]={"Extrair DVD (XEX)","DVD para GOD","Importar GOD/XBLA","NXE / Sistema"};
    for(int i=0;i<DiscMethodCount;++i){
        const int x=45+i*297;
        panel(c,x,389,286,43,Panel,i==discMode_);
        c.Text(x+10,397,modes[i],20,i==discMode_?Lime:White,266);
    }
    if(discKnown_){
        char id[128];std::sprintf(id,"Title %08X | Media %08X | Disco %u/%u",discIdentity_.titleId,discIdentity_.mediaId,discIdentity_.discNumber,discIdentity_.discCount);
        c.Text(48,446,id,21,Lime,1170);
    }else c.Text(48,446,"DVD sem default.xex legivel | Y consultar novamente",21,Muted,1170);
    if(discMode_==ExtractDvd||discMode_==ConvertGod){
        static const char* drives[]={"hdd:/","usb0:/","usb1:/","usb2:/"};
        c.Text(48,486,std::string("Destino: < ")+drives[discDestination_]+" > | "+(discVolume_.available?StorageInfo::FormatBytes(discVolume_.writableBytes)+" livres":"Indisponivel"),22,White,1170);
        c.Text(48,520,discMode_==ConvertGod?"Cria pacote GOD na biblioteca deste dispositivo. A montagem depende do XAM.":"Copia os arquivos do DVD para a pasta Games.",18,Muted,1170);
    }else if(discMode_==ImportPackage){
        c.Text(48,486,"Importa pacote existente de UserFiles/PackageImport.",22,White,1170);
        c.Text(48,520,"Informe nome em infinity360-package-import.txt.",18,Muted,1170);
    }else{
        c.Text(48,486,"Retornar ao sistema para instalar pelo recurso oficial.",22,White,1170);
        c.Text(48,520,"Nao inicia instalacao automaticamente; destino depende da configuracao de boot.",18,Muted,1170);
    }
    InfinityButtons::Strip(c,48,565,discConfirm_?"A Confirmar | B Cancelar":"Esq/Dir Metodo | Cima/Baixo Destino | Y Atualizar | A Selecionar",19,Lime,1170);
    InfinityButtons::Strip(c,48,594,"Extrair e converter usam as permissoes de DVD e escrita | B Voltar",18,Muted,1170);
#else
    c.Text(48,400,"Instalador DVD disponivel no XSDK.",22,Muted,1170);
#endif
}
void ConsoleHome::DrawTelemetry(HomeCanvas& c) const{
    if(thermalPage_){DrawThermal(c);return;}
    if(telemetryVideo_){
        c.Text(45,349,c.Tr("Telemetria 2/2 | Video e kernel"),27,White,1170);
        const auto& v=videoTelemetry_;
        const std::string labels[]={"Saida configurada","Frequencia","Proporcao","Varredura","Display kernel","Buffer kernel","Kernel","Modo HD"};
        const std::string values[]={
            v.available?number(v.width)+" x "+number(v.height):"Indisponivel",
            v.Frequency(),v.available?(v.widescreen?"16:9":"4:3"):"-",
            v.available?(v.interlaced?"Entrelacada":"Progressiva"):"-",
            v.displayAvailable?number(v.displayWidth)+" x "+number(v.displayHeight):"Indisponivel",
            v.displayAvailable?number(v.bufferWidth)+" x "+number(v.bufferHeight):"Indisponivel",
            v.kernel.empty()?"Indisponivel":v.kernel,v.available?(v.hiDef?"Sim":"Nao"):"-"};
        for(int i=0;i<8;++i){
            const int x=45+(i%4)*300,y=390+(i/4)*78;
            panel(c,x,y,289,70,Panel,false);
            c.Text(x+9,y+5,c.Tr(labels[i].c_str()),17,Muted,270);
            c.Text(x+9,y+30,values[i],22,White,270);
        }
        InfinityButtons::Strip(c,48,552,"Margem segura: < "+number(safeArea_)+"% > por lado | Esq/Dir Ajustar | X Zerar",20,Lime,1170);
        c.Text(48,580,v.available&&v.width==1920&&v.height==1080&&!v.interlaced?"Saida 1080p detectada | UI 720p escalonada pelo console":"UI 720p | Para 1080p, configure a saida no menu oficial Xbox",18,Muted,1170);
        InfinityButtons::Strip(c,48,606,c.Tr("LB/RB Pagina | Y Atualizar | B Voltar"),17,Lime,1170);
        return;
    }
    c.Text(45,349,c.Tr("Telemetria 1/2 | Memoria e sensores"),27,White,1170);
    if(telemetry_.available){
        const uint64_t bytes[]={telemetry_.physical,telemetry_.kernel,telemetry_.titlePhysical,
            telemetry_.titleAvailable,telemetry_.systemPhysical,telemetry_.titleHeap,telemetry_.titleStack,telemetry_.titleImage};
        const char* labels[]={"RAM fisica","Kernel","Titulo: fisica","Titulo: disponivel","Sistema: fisica","Titulo: heap","Titulo: stack","Titulo: imagem"};
        for(int i=0;i<8;++i){
            const int x=45+(i%4)*300,y=390+(i/4)*78;
            panel(c,x,y,289,70,Panel,false);
            c.Text(x+9,y+5,c.Tr(labels[i]),17,Muted,270);
            c.Text(x+9,y+30,StorageInfo::FormatBytes(bytes[i]),22,White,270);
        }
    }else{
        c.Box(45,391,1190,150,Panel,10);
        c.Text(65,420,std::string(c.Tr("Memoria: provider indisponivel ou consulta recusada.")),23,White,1145);
        c.Text(65,467,std::string(c.Tr("Nenhuma medida simulada e exibida.")),19,Muted,1145);
    }
    InfinityButtons::Strip(c,48,559,std::string(c.Tr("START Sensores SMC | Consulta experimental somente leitura")),18,Muted,1170);
    InfinityButtons::Strip(c,48,585,std::string(c.Tr("LB/RB Pagina | Y Atualizar | B Voltar")),18,Lime,1170);
}
void ConsoleHome::DrawThermal(HomeCanvas& c)const{
    if(coolingPage_){DrawCooling(c);return;}
    c.Text(45,349,std::string(c.Tr("Temperaturas SMC | Experimental")),27,White,1170);
    const char* labels[]={"CPU","GPU","EDRAM","Chassi"};
    for(unsigned i=0;i<4;++i){
        const int x=45+int(i)*300;
        panel(c,x,396,289,121,Panel,false);
        InfinityIcons::Draw(c,43,x+246,405,28,thermal_.available[i]?Lime:Muted);
        c.TextFit(x+12,405,c.Tr(labels[i]),19,Muted,220);
        c.Text(x+12,432,thermal_.Celsius(i),27,thermal_.available[i]?Lime:Muted,260);
        c.Text(x+12,471,"Min: "+thermalHistory_.Min(i),17,White,260);
        c.Text(x+12,493,"Max: "+thermalHistory_.Max(i),17,White,260);
    }
    const char* state=!ThermalTelemetry::Supported()?"Provider nao incluido nesta build.":thermal_.state==ThermalTelemetry::NotRead?"A solicita leitura manual.":thermal_.state==ThermalTelemetry::Valid?"Resposta SMC valida; leitura instantanea.":thermal_.state==ThermalTelemetry::Partial?"Resposta parcial; sensores ausentes nao estimados.":"Resposta invalida ou indisponivel.";
    c.Text(48,526,c.Tr(state),20,White,1170);
    c.Text(48,554,"Sessao: "+number(thermalHistory_.attempts)+" consultas | "+number(thermalHistory_.valid)+" validas | "+number(thermalHistory_.partial)+" parciais | "+number(thermalHistory_.invalid)+" invalidas",18,Muted,1170);
    InfinityButtons::Strip(c,48,580,std::string(c.Tr("LB/RB Refrigeracao | Min/max apenas de leituras manuais.")),17,Muted,1170);
    InfinityButtons::Strip(c,48,607,std::string(c.Tr("A Consultar | X Limpar historico | START Telemetria | B Voltar")),18,Lime,1170);
}
void ConsoleHome::DrawCooling(HomeCanvas& c)const{
    c.Text(45,349,std::string(c.Tr("SMC | Diagnostico de refrigeracao")),27,White,1170);
    panel(c,45,394,585,110,Panel,false);
    c.Text(62,406,std::string(c.Tr("Alvo calculado pelo SMC")),20,Muted,550);
    c.Text(62,437,thermal_.FanTargetLabel(),34,thermal_.fanTargetAvailable?Lime:Muted,550);
    c.Text(62,478,std::string(c.Tr("Nao representa RPM nem override ativo.")),17,Muted,550);
    panel(c,645,394,585,110,Panel,false);
    c.Text(662,406,std::string(c.Tr("Limite de aviso (sessao)")),20,Muted,550);
    c.Text(662,437,thermalLimit_?number(thermalLimit_)+" C":"Desativado",30,White,550);
    c.Text(662,478,std::string(c.Tr("Esq/Dir: alterar limite escolhido")),17,Muted,550);
    const auto status=ThermalAdvisory::Evaluate(thermal_,thermalLimit_);
    const char* message=status==ThermalAdvisory::Reached?"ATENCAO: limite escolhido atingido em pelo menos um sensor.":
        status==ThermalAdvisory::Below?"Ultima amostra: abaixo do limite escolhido.":
        status==ThermalAdvisory::Incomplete?"Amostra parcial: sensores ausentes impedem conclusao.":
        status==ThermalAdvisory::NoReading?"Sem amostra valida para avaliar.":"Avisos desativados.";
    c.Text(48,525,c.Tr(message),20,status==ThermalAdvisory::Reached?0xffffb454u:White,1170);
    c.Text(48,553,std::string(c.Tr("Aviso apenas na leitura manual; nao substitui a protecao do hardware.")),18,Muted,1170);
    c.Text(48,579,std::string(c.Tr("Nenhuma escrita SMC; controle real da ventoinha indisponivel.")),17,Muted,1170);
    InfinityButtons::Strip(c,48,607,std::string(c.Tr("A Consultar | X Limpar amostras | LB/RB Sensores | START Telemetria")),18,Lime,1170);
}
bool ConsoleHome::PowerBusy()const{
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMB_MENU)
    if(smbUi_!=SmbClosed)return true;
#endif
    return TransferRunning()||ftp_.Running()||peerProbe_.Running()||smbProbe_.Running()||
        nativeStorePending_||nativeBatch_||storeInstallPending_||providerPending_||
        discPending_||discSystemExit_||packageImportPending_||backupPending_||
        recoverPending_||contentInstallPending_||artPending_||coverBatchPending_||
        updatePending_||unityPending_||unityDownloadPending_;
}
void ConsoleHome::DrawPower(HomeCanvas& c)const{
    c.Text(45,349,std::string(c.Tr("Console | Energia experimental")),27,White,1170);
    const char* names[]={"Desligar console","Reiniciar console"};
    for(int i=0;i<2;++i){
        const int x=45+i*600;
        const bool selected=int(powerConfirm_.action)==i;
        panel(c,x,400,585,96,Panel,selected);
        InfinityIcons::Draw(c,i?59:11,x+20,421,44,selected?Lime:White);
        c.TextFit(x+84,426,c.Tr(names[i]),26,White,480);
    }
    const bool supported=ConsolePower::Supported(),busy=PowerBusy();
    c.Text(48,520,!supported?std::string(c.Tr("Provider nao incluido nesta build.")):busy?std::string(c.Tr("Bloqueado: encerre operacoes e FTP.")):std::string(c.Tr("Provider experimental: nao homologado no console.")),20,White,1170);
    c.Text(48,556,powerConfirm_.armed?std::string(c.Tr("CONFIRMACAO: A executa a acao selecionada; B cancela.")):std::string(c.Tr("Nenhuma acao automatica; exige dupla confirmacao.")),19,Muted,1170);
    InfinityButtons::Strip(c,48,607,std::string(c.Tr("Esq/Dir Selecionar | A Confirmar | B Voltar")),18,Lime,1170);
}
void ConsoleHome::DrawConsoleStatus(HomeCanvas& c) const{
    if(appearanceEditor_){if(layoutPreview_)DrawModernHome(c,layoutDraft_,true);else DrawAppearance(c);return;}
    if(achievementsPage_){DrawAchievements(c);return;}
    if(saveCenter_){DrawSaveCenter(c);return;}
    if(powerPage_){DrawPower(c);return;}
    if(localMilestones_){DrawLocalMilestones(c);return;}
    c.Text(45,349,c.Tr("Console | Perfis conectados"),27,White,1170);
    c.Text(48,389,std::string(c.Tr("Hora local: "))+ClockLabel(std::time(nullptr))+" | "+ClockWeather::Zone(clockSettings_.offset)+std::string(c.Tr(" | Idioma: "))+status_.language,21,Muted,1170);
    for(int i=0;i<4;++i){
        const int x=45+(i%2)*600,y=430+(i/2)*65;
        panel(c,x,y,585,58,Panel,false);
        c.Text(x+12,y+3,std::string(c.Tr("Controle "))+number(i+1)+" | "+(status_.users[i].empty()?c.Tr("Sem perfil"):status_.users[i]),20,White,560);
        c.Text(x+12,y+30,c.Tr(ConsoleStatus::StateLabel(status_.profileProvider,status_.signin[i])),16,Muted,550);
    }
    InfinityButtons::Strip(c,48,565,std::string(c.Tr("Tema "))+number(theme_+1)+"/"+number(HomeCanvas::ThemeCount)+": < "+HomeCanvas::ThemeName(theme_)+c.Tr(" > | Esq/Dir Alterar"),19,Lime,1170);
    c.Text(48,612,std::string("LB/RB Language: ")+UiLanguage::Name(language_)+" | "+c.Tr("Cima/Baixo Marcos locais")+" | A Energia",18,Lime,1170);
    InfinityButtons::Strip(c,48,590,std::string(c.Tr("X Protetor: "))+(idleMinutes_?number(idleMinutes_)+" min":c.Tr("Desativado"))+c.Tr(" | START Capas em lote | R3 Atualizar | B Voltar"),18,Lime,1170);
}
void ConsoleHome::DrawLocalMilestones(HomeCanvas& c)const{
    c.Text(45,349,c.Tr("Marcos locais da dashboard"),27,White,1170);
    unsigned installed=0,pinned=0,favorited=0,attempted=0;unsigned long long attempts=0,failed=0;
    for(const auto& game:games_){
        if(contains(hidden_,game.executable))continue;
        ++installed;
        if(contains(pinned_,game.executable))++pinned;
        if(contains(favorites_,game.executable))++favorited;
        const auto stats=launchStats_.Get(game.executable);
        if(stats.attempts){++attempted;attempts+=stats.attempts;failed+=stats.failed;}
    }
    const char* names[]={"Jogos visiveis","Favoritos","Fixados","Titulos com tentativas","Tentativas","Falhas imediatas"};
    const unsigned long long values[]={installed,favorited,pinned,attempted,attempts,failed};
    for(unsigned i=0;i<6;++i){
        const int x=45+(i%3)*400,y=393+(i/3)*75;
        panel(c,x,y,388,68,Panel,false);
        c.Text(x+10,y+4,c.Tr(names[i]),18,Muted,365);
        c.Text(x+10,y+28,std::to_string(values[i]),26,Lime,365);
    }
    c.Text(48,552,c.Tr("Indicadores locais; nao sao conquistas oficiais nem Gamerscore."),19,White,1170);
    InfinityButtons::Strip(c,48,579,std::string(c.Tr("A Conquistas GPD offline | Sem sincronizacao Xbox Live")),19,Muted,1170);
    InfinityButtons::Strip(c,48,606,std::string(c.Tr("START Central de saves | X Aparencia | Cima/Baixo Perfis | B Voltar")),18,Lime,1170);
}
void ConsoleHome::DrawStorage(HomeCanvas& c) const {
    c.Text(45,349,c.Tr("Dispositivos e espaco disponivel"),27,White,1170);
    for(size_t i=0;i<4 && storagePage_*4+i<storageVolumes_.size();++i) {
        const size_t index=storagePage_*4+i;
        const StorageVolume& v=storageVolumes_[index];
        const int x=42+(static_cast<int>(i)%2)*602;
        const int y=391+(static_cast<int>(i)/2)*106;
        c.Box(x,y,589,97,Panel,10);
        const std::string label=index==0?std::string(c.Tr("Pasta INFINITY 360")):v.path;
        InfinityIcons::Draw(c,14,x+14,y+9,22,White);
        c.TextFit(x+45,y+7,label,22,White,520);
        if(!v.available) {
            c.Text(x+14,y+44,std::string(c.Tr("Indisponivel / capacidade nao informada")),18,Muted,552);
            continue;
        }
        const unsigned percent=v.UsedPercent();
        c.Text(x+14,y+34,StorageInfo::FormatBytes(v.freeBytes)+std::string(c.Tr(" livres / "))+
               StorageInfo::FormatBytes(v.capacity)+" total",18,Muted,552);
        c.Box(x+14,y+63,551,10,0xff28413a,5);
        if(percent)c.Box(x+14,y+63,static_cast<int>(551*percent/100),10,Lime,5);
        c.Text(x+14,y+76,number(percent)+std::string(c.Tr("% ocupado")),15,White,550);
    }
    InfinityButtons::Strip(c,45,610,"LB/RB Pagina "+number(storagePage_+1)+"/"+number((storageVolumes_.size()+3)/4)+" | Y Atualizar | B Voltar | Volumes podem compartilhar disco.",18,Muted,1180);
}
void ConsoleHome::DrawSmb(HomeCanvas& c)const{
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMB_MENU)
    if(smbUi_!=SmbClosed){DrawSmbSession(c);return;}
#endif
    c.Text(45,349,std::string(c.Tr("Rede | SMB2/3 experimental")),27,White,1170);
    if(smbEditing_){
        c.Text(48,392,std::string(c.Tr("IPv4 privado do servidor SMB")),23,White,1170);
        for(unsigned i=0;i<4;++i){
            const int x=48+int(i)*175;
            panel(c,x,438,155,67,Panel,i==smbOctet_);
            c.Text(x+14,451,number(smbDraft_[i]),30,i==smbOctet_?Lime:White,130);
        }
        c.Text(48,535,std::string(c.Tr("Esq/Dir Campo | Cima/Baixo Valor | A Salvar | B Cancelar")),19,Lime,1170);return;
    }
    LanPeerConfig::Octets configured{};
    const bool known=LanPeerConfig::Read(dir_+"infinity360-smb-peer.txt",configured);
    c.Text(48,392,std::string(c.Tr("Servidor: "))+(known?LanPeerConfig::Format(configured):std::string(std::string(c.Tr("Nao configurado"))))+" | TCP 445",22,White,1170);
    c.Text(48,435,(smbProbe_.Peer().empty()?std::string(""):smbProbe_.Peer()+" | ")+c.Tr(smbProbe_.Label().c_str()),22,smbProbe_.Status()==SmbProbe::Negotiated?Lime:White,1170);
    const auto& result=smbProbe_.Info();
    c.Text(48,480,std::string(std::string(c.Tr("Dialeto: ")))+SmbProbe::Dialect(result.dialect)+(smbProbe_.Status()==SmbProbe::Negotiated?(result.signingRequired?std::string(c.Tr(" | Assinatura exigida")):std::string(c.Tr(" | Assinatura suportada"))):""),21,Muted,1170);
    if(smbProbe_.Status()==SmbProbe::Rejected)c.Text(48,510,"Status NT: "+number(result.status),18,Muted,1170);
    c.Text(48,550,std::string(c.Tr("Dashboard: somente negociacao; adaptador de sessao em homologacao.")),19,Muted,1170);
    c.Text(48,579,smbProbe_.Status()==SmbProbe::Negotiated?
        std::string(result.preauthSha512?std::string(c.Tr("SHA-512 selecionado | ")):"")+std::string(c.Tr("Token de seguranca: "))+number(result.securityTokenBytes)+std::string(c.Tr(" bytes | Sessao nao autenticada")):
        std::string(c.Tr("SMB 2.0.2 a 3.1.1 | Sem SMB1 ou acesso anonimo.")),17,Muted,1170);
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMB_MENU)
    InfinityButtons::Strip(c,48,607,std::string(c.Tr("A Testar | X Cancelar | START IP | R3 Sessao | B Voltar")),18,Lime,1170);
#else
    InfinityButtons::Strip(c,48,607,std::string(c.Tr("A Testar | X Cancelar | START Configurar | B Voltar")),18,Lime,1170);
#endif
}
void ConsoleHome::DrawNetwork(HomeCanvas& c) const {
    if(smbPage_){DrawSmb(c);return;}
    if(networkPeer_){
        c.Text(45,349,c.Tr("Rede 2/2 | Diagnostico UDP entre consoles"),27,White,1170);
        if(peerOptions_){
            const unsigned durations[]={15,30,60};
            c.Text(48,397,std::string(c.Tr("Modo: "))+std::string(c.Tr(LanPeerProbe::ModeLabel(static_cast<LanPeerProbe::Mode>(peerMode_)))),25,Lime,1170);
            c.Text(48,445,std::string(c.Tr("Duracao: "))+number(durations[peerDuration_])+std::string(c.Tr(" segundos")),25,White,1170);
            c.Text(48,492,std::string(c.Tr("Anfitriao responde | Cliente envia | Bidirecional envia e responde")),19,Muted,1170);
            c.Text(48,550,std::string(c.Tr("Protocolo de diagnostico INFL; nao cria sala de jogo.")),18,Muted,1170);
            InfinityButtons::Strip(c,48,607,std::string(c.Tr("Esq/Dir Modo | Cima/Baixo Duracao | A Aplicar | B Voltar")),18,Lime,1170);return;
        }
        if(peerEditing_){
            c.Text(48,392,std::string(c.Tr("IPv4 privado do outro console")),23,White,1170);
            for(unsigned i=0;i<4;++i){
                const int x=48+static_cast<int>(i)*175;
                panel(c,x,438,155,67,Panel,i==peerOctet_);
                c.Text(x+14,451,number(peerDraft_[i]),30,i==peerOctet_?Lime:White,130);
            }
            c.Text(48,535,std::string(c.Tr("Esq/Dir Campo | Cima/Baixo Valor | A Salvar | B Cancelar")),19,Lime,1170);return;
        }
        c.Text(48,392,std::string(c.Tr("Local: "))+(network_.ipv4.empty()?"-":network_.ipv4)+std::string(c.Tr(" | Par: "))+(peerProbe_.Peer().empty()?"-":peerProbe_.Peer())+" | UDP 32160",22,White,1170);
        c.Text(48,433,c.Tr(peerProbe_.Label().c_str()),23,Lime,1170);
        c.Text(48,475,std::string(c.Tr("Tentativas: "))+number(peerProbe_.Attempts())+std::string(c.Tr(" | Enviados: "))+number(peerProbe_.Sent())+std::string(c.Tr(" | Respostas: "))+number(peerProbe_.Received())+(peerProbe_.Received()?" | RTT "+number(peerProbe_.Rtt())+" ms":""),22,White,1170);
        c.Text(48,513,peerProbe_.Received()?std::string(c.Tr("RTT min/media/max: "))+number(peerProbe_.MinRtt())+"/"+number(peerProbe_.MeanRtt())+"/"+number(peerProbe_.MaxRtt())+std::string(c.Tr(" ms | Variacao: "))+(peerProbe_.Received()>1?number(peerProbe_.Jitter())+" ms":"--"):std::string(c.Tr("RTT indisponivel | Pedidos: "))+number(peerProbe_.Requests())+std::string(c.Tr(" | Respostas enviadas: "))+number(peerProbe_.Replies()),19,Muted,1170);
        const unsigned durations[]={15,30,60};
        c.Text(48,540,std::string(c.Tr(LanPeerProbe::ModeLabel(static_cast<LanPeerProbe::Mode>(peerMode_))))+" | "+number(peerProbe_.Elapsed())+"/"+number(durations[peerDuration_])+" s"+(peerProbe_.Sent()&&peerProbe_.Outcome()==LanPeerProbe::Finished?std::string(c.Tr(" | Sem resposta: "))+number(peerProbe_.LossPercent())+"%":""),18,Muted,1170);
        c.Text(48,560,std::string(c.Tr("Inicie nos dois consoles. Nao confirma System Link de jogos.")),18,Muted,1170);
        InfinityButtons::Strip(c,48,607,std::string(c.Tr("A Iniciar | X Parar | START IP | R3 Opcoes | LB/RB Pagina")),18,Lime,1170);return;
    }
    c.Text(45,349,c.Tr("Rede 1/2 | Diagnostico de rede local"),27,White,1170);
    c.Box(42,391,589,190,Panel,10);
    c.Text(58,404,c.Tr("Conexao e endereco IPv4"),25,White,550);
    c.Text(58,447,c.Tr(NetworkInfo::StateLabel(network_.state)),20,Muted,550);
    c.Text(58,486,"IP: "+(network_.ipv4.empty()?std::string(std::string(c.Tr("Nao identificado"))):network_.ipv4),
           23,network_.ipv4.empty()?Muted:Lime,550);
    c.Text(58,534,network_.ethernetKnown?
           (network_.ethernetActive?"Ethernet: ativo"+(network_.linkMbps?" | "+number(network_.linkMbps)+" Mbps":"")+(network_.duplex.empty()?"":" | "+network_.duplex):"Ethernet: sem link"):
           "Ethernet: leitura indisponivel",19,Muted,550);
    c.Box(644,391,594,190,Panel,10);
    c.Text(660,404,c.Tr("Servicos de rede"),25,White,558);
    c.Text(660,447,ftp_.Running()?(ftp_.Connected()?std::string(c.Tr("FTP: cliente conectado")):std::string(c.Tr("FTP: aguardando cliente"))):std::string(c.Tr("FTP: desligado")),21,ftp_.Running()?Lime:Muted,558);
    c.Text(660,486,ftp_.Running()?std::string(c.Tr("Porta "))+number(ftp_.Port())+" | "+(ftp_.UploadEnabled()?std::string(c.Tr("Leitura/upload")):std::string(c.Tr("Somente leitura"))):std::string(c.Tr("A Leitura | X Leitura/upload (ALLO)")),20,Muted,558);
    c.Text(660,534,std::string(c.Tr("Internet e Xbox Live: nao testados")),19,Muted,558);
    InfinityButtons::Strip(c,45,607,std::string(c.Tr("A Leitura | X Upload | A/X Parar | LB/RB UDP | START SMB | Y Atualizar | B Voltar")),18,Muted,1180);
}
void ConsoleHome::DrawEvents(HomeCanvas& c) const {
    const auto rows=notifications_.Visible(eventFilter_);
    static const char* filters[]={"Todas","Nao lidas","Atencao"};
    c.Text(45,349,std::string(c.Tr("Notificacoes"))+" | "+number(notifications_.UnreadCount())+" "+c.Tr("nao lidas"),26,White,1170);
    for(unsigned i=0;i<NotificationCenter::FilterCount;++i){
        const int x=48+static_cast<int>(i)*210;
        panel(c,x,387,200,31,Panel,i==static_cast<unsigned>(eventFilter_));
        c.Text(x+9,389,c.Tr(filters[i]),17,i==static_cast<unsigned>(eventFilter_)?Lime:Muted,180);
    }
    if(rows.empty())c.Text(48,455,c.Tr("Nenhuma notificacao neste filtro."),23,Muted,1160);
    const int first=(eventRow_/4)*4;
    for(int i=first;i<first+4&&static_cast<size_t>(i)<rows.size();++i){
        const auto& item=notifications_.At(rows[i]);const int y=426+(i-first)*42;
        panel(c,48,y,1184,38,Panel,i==eventRow_);
        const unsigned tone=item.level==NotificationCenter::Warning?0xffe4a34bu:item.level==NotificationCenter::Success?Lime:Muted;
        c.Box(57,y+9,6,19,item.read?Muted:tone,2);
        c.Text(72,y+3,c.Tr(item.area.c_str()),17,tone,125);
        c.Text(203,y+3,item.text,17,White,825);
        c.Text(1044,y+3,c.Tr(item.read?"Lida":"Nova"),17,item.read?Muted:Lime,170);
    }
    InfinityButtons::Strip(c,48,600,c.Tr("LB/RB Filtro | A Ler | X Ler todas | Cima/Baixo | B Voltar"),18,Lime,1175);
}
void ConsoleHome::RebuildStoreDownloads() {
    storeDownloadItems_=store_.NativeQueue();
    for(size_t i=0;i<store_.Items().size();++i)
        if(store_.ListedDownload(store_.Items()[i])&&std::find(storeDownloadItems_.begin(),storeDownloadItems_.end(),i)==storeDownloadItems_.end())storeDownloadItems_.push_back(i);
    clampRow(storeDownloadRow_,storeDownloadItems_.size());
}
void ConsoleHome::DrawStoreDownloads(HomeCanvas& c) const {
    const auto summary=store_.QueueSummary();
    c.Text(45,346,"Downloads | "+StoreStorage::Label(store_.Storage()),25,White,540);
    c.Text(590,346,store_.NativeQueueValid()?number(summary.queued)+" na fila | "+number(summary.paused)+" parciais | "+StorageInfo::FormatBytes(summary.remaining)+" restantes":"Fila invalida; arquivo preservado",18,Muted,640);
    c.Text(590,370,number(summary.verified)+" verificados | "+number(summary.unavailable)+" referencias ausentes",15,Muted,640);
    const int first=(storeDownloadRow_/4)*4;
    for(int i=first;i<first+4&&static_cast<size_t>(i)<storeDownloadItems_.size();++i) {
        const StoreItem& item=store_.Items()[storeDownloadItems_[i]];
        const StoreProgress status=store_.Progress(item);
        const int y=390+(i-first)*48;
        panel(c,45,y,1189,43,Panel,i==storeDownloadRow_);
        c.Text(60,y+8,item.name,20,White,650);
        const std::string label=status.state==StoreProgress::NativeVerified?"Verificado no Xbox (registro)":status.state==StoreProgress::Verified?"Verificado pelo PC":
            status.state==StoreProgress::NativePaused?"Pausado "+number(static_cast<unsigned>(100.0*status.received/item.bytes))+"%":
            status.state==StoreProgress::Failed?"Ultima tentativa falhou":
            status.state==StoreProgress::Receiving?"Ultimo progresso "+number(static_cast<unsigned>(100.0*status.received/item.bytes))+"%":
            store_.NativeQueued(item)?"Na fila nativa":"Pedido PC / sem estado";
        c.Text(745,y+8,label,18,Muted,465);
    }
    if(storeDownloadItems_.empty())c.Text(48,415,"Nenhum download ou pedido no catalogo atual.",22,Muted,1160);
#if defined(INFINITY_XSDK_HTTPS)
    InfinityButtons::Strip(c,48,590,"A Baixar/Retomar | START Rodar fila | R3 Priorizar | X Remover",18,Muted,1170);
    InfinityButtons::Strip(c,48,612,"LB Limpar ausentes ("+number(store_.OrphanNativeCount())+") | RB Copias locais | Y Atualizar | B Voltar",18,Lime,1170);
#else
    c.Text(48,590,"Registros locais do Xbox e PC; nao indicam conexao ativa.",18,Muted,1170);
    InfinityButtons::Strip(c,48,612,"RB Copias locais | Y Atualizar | B Voltar",18,Lime,1170);
#endif
}
void ConsoleHome::DrawTransfers(HomeCanvas& c) const {
    if(storeDownloads_) {DrawStoreDownloads(c);return;}
    c.Text(45,349,"Transferencias locais | RB Downloads da loja",27,White,1170);
    c.Box(42,391,1196,202,Panel,10);
    const FileTransfer::State state=transfer_.Status();
    if(state==FileTransfer::Idle) {
        c.Text(60,422,"Nenhuma transferencia nesta sessao.",24,White,1150);
        c.Text(60,476,"Arquivos: X Copiar, LB Colar e A Confirmar.",20,Muted,1150);
    } else {
        const char* const states[]={"Inativo","Copiando","Concluido","Cancelado","Falhou"};
        c.Text(60,406,transfer_.Name(),24,White,1145);
        c.Text(60,450,(copyPaused_?"Pausado":states[state])+std::string(" | ")+
               StorageInfo::FormatBytes(transfer_.Done())+" / "+
               StorageInfo::FormatBytes(transfer_.Total()),20,Muted,1145);
        c.Box(60,493,1155,16,0xff28413a,5);
        const unsigned percent=transfer_.Percent();
        if(percent)c.Box(60,493,static_cast<int>(1155*percent/100),16,Lime,5);
        c.Text(60,526,number(percent)+"%"+(state==FileTransfer::Running&&!copyPaused_?" | "+copyRate_.Label(transfer_.Done(),transfer_.Total()):std::string(""))+(transfer_.Error().empty()?
               std::string(""):" | "+transfer_.Error()),20,White,1145);
    }
    c.Text(60,562,"Na fila: "+number(copyQueue_.size())+
           (copyQueue_.empty()?"":" | Proximo: "+copyQueue_.front().name),18,Muted,1145);
    c.Text(45,609,TransferRunning()?
           (copyPaused_?"A Retomar | X Cancelar fila | B Voltar":"A Pausar | X Cancelar fila | B Voltar"):
           "B Voltar | Downloads de internet no PC",18,Muted,1180);
}
void ConsoleHome::DrawArtEditor(HomeCanvas& c) const {
    c.Text(44,78,"Personalizar arte",40,White,1190);
    if(artEditRow_==2){
        c.TextFit(46,137,"Title ID: "+artEditId_+" | Verso da capa",21,Muted,1180);
        c.SeriesTile(45,180,370,375,true);
        const auto path=Count()?ReverseArtPath(*visible_[row_]):std::string();
        if(path.empty()||!c.LocalArt(path,true,130,210,200,300,false,8))c.CenterText(55,330,350,"Sem verso",23,Muted);
        c.SeriesTile(430,180,805,375,false);
        c.TextFit(455,207,"Capa traseira",28,White,740);
        c.TextFit(455,277,"UserFiles/ArtImport/"+artEditId_+"/cover-back.jpg",18,White,740);
        c.TextFit(455,310,"ou cover-back.png | Importacao sem sobrescrita",18,Muted,740);
        c.TextFit(455,380,"Na Home, B alterna a frente e o verso do jogo selecionado.",18,White,740);
        InfinityButtons::Strip(c,46,570,"Cima/Baixo Selecionar | R3 Importar | RB Recarregar",19,White,1180);
        InfinityButtons::Strip(c,46,604,"Y Salvar | B Voltar",18,Lime,1180);return;
    }
    c.Text(46,137,"Title ID: "+artEditId_,21,Muted,1180);
    static const char* const modes[]={"Principal","Alternativa","Visual padrao"};
    for(int i=0;i<2;++i) {
        const int x=i?430:45, w=i?805:370;
        panel(c,x,180,w,375,Panel,i==artEditRow_);
        c.Text(x+16,190,i?"Fundo":"Capa",27,White,w-32);
        std::string file=artFile(artEditFolder_,i==0,artDraft_[i]);
        if(artDraft_[i]=='0'&&!fileExists(file))file=FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(dir_,"UserFiles"),"Art"),artEditId_),i?"background.rgba":"cover.rgba");
        const bool drawn=!file.empty() && (i?
            c.LocalArt(file,false,x+190,244,400,225):
            c.LocalArt(file,true,x+105,244,150,225));
        if(!drawn)c.Text(x+20,330,artDraft_[i]=='2'?"Visual original":"Arquivo ausente ou invalido",21,Muted,w-40);
        c.Text(x+16,488,std::string("< ")+modes[artDraft_[i]-'0']+" >",24,Lime,w-32);
        c.Text(x+16,523,artDraft_[i]=='2'?"Sem arte personalizada":i?
            (artDraft_[i]=='1'?"background-alt.rgba":"background.rgba"):
            (artDraft_[i]=='1'?"cover-alt.rgba":"cover.rgba"),17,Muted,w-32);
    }
    InfinityButtons::Strip(c,46,570,"Cima/Baixo Frente/Fundo/Verso | Esq/Dir ou A Alterar | RB Recarregar",19,White,1180);
    InfinityButtons::Strip(c,46,604,artArchiveConfirm_?"A Confirmar | B Cancelar":"START Capa XboxUnity | R3 Importar | LB Arquivar/Restaurar | Y Salvar | B Voltar",18,Lime,1180);
}
void ConsoleHome::DrawFileActions(HomeCanvas& c) const {
    c.Text(45,145,"Acoes do arquivo",31,White,1180);
    c.Text(48,196,actionName_,24,Lime,1160);
    const char* const actions[]={"Recortar arquivo","Excluir item",
        multiSelect_ && copyDir_==actionDir_?"Limpar multisselecao":"Iniciar multisselecao","Voltar"};
    for(int i=0;i<4;++i) {
        const int y=240+i*72;
        panel(c,45,y,1189,60,Panel,i==fileActionRow_);
        const int icons[]={56,55,57,59};InfinityIcons::Draw(c,icons[i],61,y+12,32,White);
        c.TextFit(110,y+12,actions[i],25,White,1070);
    }
    c.Text(48,550,"Somente espaco pessoal. Exclusao nao recursiva; movimento no mesmo volume.",20,Muted,1175);
    InfinityButtons::Strip(c,48,600,deleteConfirm_?"A Confirmar exclusao definitiva | B Cancelar":"A Selecionar | B Voltar",20,Lime,1175);
}
void ConsoleHome::DrawDiscSet(HomeCanvas& c) const{
    c.Text(44,78,"Jogos multidisco | Selecionar disco",37,White,1170);
    c.Text(48,136,"Title ID "+hexId(discSetIdentity_.titleId)+" | Base "+hexId(discSetIdentity_.baseVersion)+" | "+number(discSetIdentity_.discCount)+" discos",21,Muted,1170);
    std::set<unsigned> present;for(const auto& e:discSet_)present.insert(e.info.discNumber);
    c.Text(48,177,number(present.size())+" de "+number(discSetIdentity_.discCount)+" discos localizados | "+number(discSet_.size())+" instalacoes",20,Muted,1170);
    const size_t first=discSetRow_/5*5;
    for(size_t i=first;i<discSet_.size()&&i<first+5;++i){
        const auto& e=discSet_[i];const int y=220+static_cast<int>(i-first)*70;
        panel(c,45,y,1189,62,Panel,i==discSetRow_);
        c.Text(60,y+4,"Disco "+number(e.info.discNumber)+" | "+e.name+" | "+e.device,23,White,1150);
        c.Text(60,y+32,e.path+" | Media "+hexId(e.info.mediaId),17,Muted,1150);
    }
    if(discSet_.empty())c.Text(48,270,"Nenhum disco correspondente instalado.",24,Muted,1170);
    InfinityButtons::Strip(c,48,590,"A Selecionar XEX | Y Revalidar | B Voltar | Sem troca automatica",19,Lime,1170);
}
void ConsoleHome::DrawSection(HomeCanvas& c) const {
    if(updateActive_){DrawDashboardUpdate(c);return;}
    if(discSetActive_){DrawDiscSet(c);return;}
    if(artEditing_) {DrawArtEditor(c);return;}
    if(contentActive_) {DrawContent(c);return;}
    const int sectionIcons[]={23,3,2,1,InfinityIcons::Folder,10,20,21};
    InfinityIcons::Draw(c,sectionIcons[section_],46,91,32,White);
    c.TextFit(91,78,c.Tr(labels[section_]),40,White,1140);
    if(section_==Files && page_==Listing && fileAction_) {DrawFileActions(c);return;}
    if((section_==Library || section_==Files) && page_==Listing && searchActive_) {
        c.Text(45,139,keyboardMode_==1?"Nova colecao":
               keyboardMode_==2?"Renomear colecao":keyboardMode_==3?"Nova pasta":
               keyboardMode_==4?"Renomear item":"Buscar jogos instalados",
               26,White,1100);
        c.Box(42,176,1196,49,Panel,9);
        c.Text(58,181,searchDraft_.empty()?"Digite um nome...":searchDraft_,
               26,Lime,1156);
        for(int i=0;i<KeyCount;++i) {
            const int x=45+(i%7)*171, y=255+(i/7)*59;
            panel(c,x,y,157,48,Panel,i==searchCursor_);
            c.Text(x+13,y+8,keyboardKeys[i],i>=36?18:23,
                   i==searchCursor_?Lime:White,140);
        }
        c.Text(48,619,section_==Files?"A Digitar | X Apagar | Y Confirmar | B Cancelar":
               "A Digitar  X Apagar  Y Buscar  B Cancelar",
               19,Muted,1160);
        return;
    }
    if(section_==Library && page_==Listing && collectionPicker_) {
        const std::vector<GameCollection>& lists=collections_.Items();
        c.Text(46,142,"Gerenciar colecoes",30,White,1050);
        c.Text(47,192,pickerGame_.empty()?
               "Escolha uma colecao ou crie uma nova.":
               "Escolha uma colecao para incluir ou remover o jogo.",
               20,Muted,1130);
        const int total=static_cast<int>(lists.size())+1;
        const int first=(collectionRow_/7)*7;
        for(int i=first;i<total && i<first+7;++i) {
            const int y=229+(i-first)*57;
            panel(c,45,y,1189,50,Panel,i==collectionRow_);
            const bool creation=static_cast<size_t>(i)==lists.size();
            const std::string name=creation?"+ Nova colecao":lists[i].name;
            const bool assigned=!creation && !pickerGame_.empty() &&
                collections_.Contains(static_cast<size_t>(i),pickerGame_);
            c.Text(63,y+7,name,25,i==collectionRow_?Lime:White,780);
            if(assigned)c.Text(886,y+9,"Jogo incluido",19,Lime,325);
            else if(!creation)c.Text(886,y+9,"Colecao local",19,Muted,325);
        }
        c.Text(47,633,deleteCollectionConfirm_?
               "Y novamente: confirmar exclusao da colecao | B cancelar":
               "A Incluir/Remover | X Renomear | Y Excluir | B Voltar",
               18,Muted,1190);
        return;
    }
    if(section_==System) {
        static const char* const titles[]={
            "Discos","Rede/FTP","Telemetria","Downloads","Plugins","Historico","DVD","Console"
        };
        static const int marks[]={14,7,43,5,42,22,13,8};
        static const char* const summaries[]={
            "Jogos e discos","Rede local","Telemetria","Tarefas locais","Residentes","Eventos","Instalar disco","Perfis e hora"
        };
        static const char* const details[]={
            "Capacidade e espaco livre dos dispositivos.",
            "FTP LAN: leitura e upload opcional; ativacao manual.",
            "Memoria do kernel; sensores e ventoinha pendentes.",
            "Transferencias locais; downloads de internet pendentes.",
            "Diagnostico somente leitura do launch.ini.",
            "Historico de operacoes da dashboard.",
            "Instalacao dos arquivos acessiveis pelo leitor DVD.",
            "Perfis Xbox, idioma e relogio do console."
        };
        if(page_==Listing){
            const char* groups[]={"Dispositivos e rede","Monitoramento e tarefas","Extensões e eventos","Instalação e console"};
            for(int g=0;g<4;++g){
                const int x=42+g*300;c.SeriesTile(x,166,289,166,false);
                c.TextFit(x+12,176,groups[g],19,White,265);
                for(int j=0;j<2;++j){const int i=g*2+j,y=212+j*55;
                    c.SeriesTile(x+8,y,273,49,i==systemTab_);
                    InfinityIcons::Draw(c,marks[i],x+18,y+10,27,i==systemTab_?Lime:White);
                    c.TextFit(x+57,y+9,c.Tr(titles[i]),20,White,215);
                }
            }
        }else{
            // A single selected category replaces eight unrelated shortcuts on details pages.
            c.SeriesTile(42,166,1196,166,false);
            InfinityIcons::Draw(c,marks[systemTab_],65,193,68,Lime);
            c.TextFit(155,185,c.Tr(titles[systemTab_]),34,White,1055);
            c.TextFit(155,242,c.Tr(summaries[systemTab_]),23,Muted,1055);
            InfinityButtons::Hint(c,InfinityButtons::B,155,285,26,"Voltar à central",250,White);
        }
        if(page_==Details && systemTab_==0) {DrawStorage(c);return;}
        if(page_==Details && systemTab_==1) {DrawNetwork(c);return;}
        if(page_==Details && systemTab_==2) {DrawTelemetry(c);return;}
        if(page_==Details && systemTab_==3) {DrawTransfers(c);return;}
        if(page_==Details && systemTab_==4) {DrawResidentPlugins(c);return;}
        if(page_==Details && systemTab_==5) {DrawEvents(c);return;}
        if(page_==Details && systemTab_==6) {DrawDiscInstall(c);return;}
        if(page_==Details && systemTab_==7) {DrawConsoleStatus(c);return;}
        if(page_==Details) {
            c.Box(42,369,1196,220,Panel,12);
            c.Text(68,391,c.Tr(titles[systemTab_]),34,White,1120);
            c.Text(69,470,details[systemTab_],23,Muted,1122);
            c.Text(69,543,"B para voltar aos paineis.",20,Lime,1120);
        } else {
            c.Text(47,369,"Resumo INFINITY 360",29,White,1100);
            c.Box(42,422,1196,165,Panel,12);
            c.Text(67,443,"Jogos encontrados    "+number(games_.size()),25,White,1120);
            c.Text(67,494,"Catalogo IPTV local   "+number(tv_.Count())+" itens",23,Muted,1120);
            c.Text(67,543,"Interface nativa      1280 x 720",22,Muted,1120);
        }
        return;
    }
    if(page_==Details && Count()) {
        if(section_==Library||section_==Recent||section_==Favorites){DrawModernGameDetails(c);return;}
        c.Box(44,169,1190,407,Panel,12);
        std::string title,sub,hint;
        if(section_==Files) {
            title=files_[row_].name; sub=folders_.back(); hint="Visualização de arquivos";
        } else {
            title=visible_[row_]->name; sub=visible_[row_]->executable;
            hint=(isXexPath(sub) && !isContentPackage(*visible_[row_]))?
                "Pressione A para iniciar este título.":
                "Pacote identificado. A solicita montagem XAM experimental (GOD/XBLA).";
        }
        const std::string backFile=section_==Files?"":ArtPath(*visible_[row_],false);
        const std::string coverFile=section_==Files?"":ArtPath(*visible_[row_],true);
        const bool backdrop=appearance_.ShowBackdrop() && !backFile.empty() && c.LocalArt(backFile,false,842,202,360,202);
        const bool cover=appearance_.ShowCover() && !coverFile.empty() && c.LocalArt(coverFile,true,1090,406,100,150);
        const int textWidth=(backdrop || cover)?740:1100;
        c.Text(76,219,title,36,White,textWidth);
        c.Text(77,283,sub,20,Muted,textWidth);
        if(section_!=Files) {
            const XboxTitleMetadata& m=detailMetadata_;
            if(m.known) {
                c.Text(77,329,"Title ID: "+hexId(m.titleId)+"    Media ID: "+
                       (m.mediaId?hexId(m.mediaId):std::string("Indisponivel")),23,White,textWidth);
                c.Text(77,368,"Versao (hex): "+hexId(m.version)+
                       (m.package?"    Tipo: "+hexId(m.contentType):"    Base: "+hexId(m.baseVersion)),21,Muted,textWidth);
                const std::string extra=m.package?
                    std::string(m.volumeType==0?"STFS":"SVOD")+" | Tamanho declarado: "+
                       StorageInfo::FormatBytes(m.declaredBytes)+(m.discCount?" | Disco "+number(m.discNumber)+"/"+number(m.discCount):""):
                    m.discCount?"Disco "+number(m.discNumber)+" de "+number(m.discCount):
                       "Disco: informacao indisponivel";
                c.Text(77,407,extra,21,Muted,textWidth);
                if(translationReferences_.count(m.titleId))
                    c.Text(77,439,"PT-BR: titulo referenciado; versao/download nao confirmados",18,Muted,textWidth);
            } else c.Text(77,351,"Metadados indisponiveis para este item.",22,Muted,1100);
        }
        c.Text(77,469,hint,21,Lime,textWidth);
        if(section_==Library || section_==Recent || section_==Favorites)
            c.Text(47,593,detailMetadata_.discCount>1?"Start Arte | R3 Selecionar disco | Y Atualizar":"Start Personalizar capa e fundo | Y Atualizar",18,Muted,1170);
        if(section_==Library || section_==Recent || section_==Favorites) {
            c.Text(77,512,"RB Conteudo instalado (TU / DLC / Saves / Trainers)",20,Muted,1110);
            c.Text(77,545,contains(pinned_,visible_[row_]->executable)?"LB Desafixar do inicio":"LB Fixar no inicio",20,Lime,1110);
            const auto stats=LaunchStats(visible_[row_]->executable);
            c.Text(47,617,stats.attempts?"Tentativas "+number(stats.attempts)+" | Falhas imediatas "+number(stats.failed)+" | Ultima "+LaunchStatistics::Date(stats.last):"Estatisticas: nenhuma tentativa registrada nesta versao",17,Muted,1170);
        }
        return;
    }
    const bool tabbed=(section_==Apps || section_==Emulators || section_==Library);
    if(tabbed) {
        static const char* const appTabs[]={"Todos","Instalados","Plugins","Homebrew"};
        static const char* const gameTabs[]={"Todos","Xbox 360","XBLA","Original","Pacotes","Colecoes","Nunca","Ocultos","Fixados"};
        const char* const* tabs=section_==Library?gameTabs:appTabs;
        const int numTabs=section_==Library?9:section_==Emulators?3:4;
        const int tabWidth=numTabs==9?124:numTabs==3?244:207;
        for(int i=0;i<numTabs;++i) {
            const int x=45+i*(tabWidth+9);
            panel(c,x,148,tabWidth,43,Panel,i==subtab_);
            const int gameIcons[]={23,1,35,13,35,24,20,49,34};
            const int appIcons[]={2,50,42,35};
            InfinityIcons::Draw(c,section_==Library?gameIcons[i]:appIcons[i],x+7,159,21,i==subtab_?Lime:White);
            c.TextFit(x+33,155,c.Tr(tabs[i]),numTabs==9?15:19,White,tabWidth-38);
            if(GridListing()&&gridFilter_&&i==subtab_)c.Box(x+4,185,tabWidth-8,3,Lime);
        }
        c.Text(49,200,number(Count())+c.Tr(" itens nesta aba"),21,Muted,1120);
        if(section_==Library) {
            static const char* const sorts[]={"Nome A-Z","Nome Z-A","Dispositivo"};
            c.Text(861,201,subtab_==8?c.Tr("Ordem manual"):std::string(c.Tr("RS Ordenar: "))+c.Tr(sorts[gameSort_]),18,Muted,355);
            if(subtab_==5 && !collections_.Items().empty()) {
                const size_t current=static_cast<size_t>(collectionIndex_);
                if(current<collections_.Items().size())
                    c.Text(315,200,collections_.Items()[current].name,21,Lime,510);
            }
            c.Text(45,621,subtab_==5?
                   "Start Buscar | LB Proxima colecao | RB Gerenciar | RS Ordenar":
                   subtab_==8?"Start Buscar | LB Ocultar | RB Subir fixado | RS Descer fixado":
                   "Start Buscar | LB Ocultar | RB Colecoes | RS Ordenar",
                   18,Muted,1170);
        }
    } else if(section_==Files) {
        c.Text(49,146,folders_.back(),21,White,1140);
        c.Text(49,176,multiSelect_ && copyDir_==folders_.back()?
            "Multisselecao: "+number(copyNames_.size())+" itens | X marca/desmarca":
            fileWorkspace_.CanWrite(folders_.back())?
            "Espaco pessoal | escrita habilitada":"Navegacao somente leitura | Start cria espaco pessoal no dispositivo",
            18,Muted,1140);
        c.Text(49,594,"Esquerda: busca de jogos | Direita: acoes do item",18,Muted,1180);
        InfinityButtons::Strip(c,49,615,"RB Dispositivo | Start Nova pasta | RS Renomear | X Copiar | LB Colar",18,Lime,1180);
    }
    else c.Text(49,157,number(Count())+" títulos",21,Muted,1140);
    if(!Count() && notice_.empty())
        c.Text(56,tabbed?275:246,
               section_==Library && subtab_==5?
               "Colecao vazia. RB gerencia colecoes ou cria uma nova.":
               "Nenhum item nesta seção.",25,Muted,1120);
    if(GridListing()){DrawGameGrid(c);return;}
    int start=row_/7*7;
    for(int i=start;i<static_cast<int>(Count()) && i<start+7;++i) {
        int y=(tabbed?240:212)+(i-start)*(tabbed?55:59);
        panel(c,45,y,1189,tabbed?49:52,Panel,i==row_);
        std::string name,detail;
        if(section_==Files) {
            name=files_[i].name; detail=files_[i].directory?"Pasta":"Arquivo";
            if(copyDir_==folders_.back() &&
               (contains(copyNames_,name)||(!copyName_.empty()&&copyName_==name)))
                detail="MARCADO | "+detail;
        } else {name=visible_[i]->name; detail=std::string(contains(pinned_,visible_[i]->executable)?"FIXADO  ":"")+(Starred(*visible_[i])?"*  ":"")+visible_[i]->device;}
        int nameX=62;
        if(section_!=Files) {
            const std::string coverFile=ArtPath(*visible_[i],true);
            if(!coverFile.empty() && c.LocalArt(coverFile,true,55,y+2,30,45))nameX=99;
        }
        if(nameX==62){InfinityIcons::Draw(c,section_==Files?14:1,57,y+10,26,Muted);nameX=96;}
        c.TextFit(nameX,y+6,name,23,White,872-nameX);
        c.TextFit(886,y+9,detail,19,Muted,336);
    }
}
void ConsoleHome::DrawSocialHeader(HomeCanvas& c,int x,int y,int w)const{
    using namespace InfinitySocial;
    c.SeriesTile(x,y,w,52,false);
    const bool signedIn=social_.controller>=0;
    // Real provider tile when available; neutral silhouette otherwise.
    const unsigned ink=signedIn?Lime:Muted;
    c.Box(x+8,y+7,38,38,0xff68716du,19);
    if(!c.ProfilePicture(social_.gamerpic,x+9,y+8,36))InfinityIcons::Draw(c,InfinityIcons::Avatar,x+9,y+8,36,White);
    if(signedIn)c.Box(x+36,y+36,9,9,ink,4);
    const char* state=social_.origin==Origin::Test?"PERFIL DE TESTE":
        social_.state==SignIn::Online?"Sessão online":
        social_.state==SignIn::Local?"Sessão local":
        social_.state==SignIn::SignedOut?"Sem perfil":"Nao consultado";
    c.TextFit(x+52,y+5,signedIn?social_.displayName:std::string("Perfil Xbox"),17,White,w-58);
    const std::string subtitle=social_.origin==Origin::Test?"PERFIL DE TESTE":
        social_.guest?"Convidado":social_.gamerscoreKnown?std::string(state)+" | G "+number(social_.gamerscore):state;
    c.TextFit(x+52,y+28,subtitle,13,ink,w-58);
}
namespace { void approvedIcon(HomeCanvas& c,int kind,int x,int y,int size,unsigned color); }
void ConsoleHome::DrawSocialGuide(HomeCanvas& c)const{
    using namespace InfinitySocial;
    c.SeriesTile(580,87,470,552,false,244);
    approvedIcon(c,8,606,110,30,White);
    c.TextFit(644,105,"Perfil e comunicação",28,White,385);
    c.SeriesTile(601,166,428,91,false,230);
    const unsigned ink=social_.controller>=0?Lime:Muted;
    c.Box(623,184,54,54,0xff68716du,27);
    if(!c.ProfilePicture(social_.gamerpic,624,185,52))InfinityIcons::Draw(c,InfinityIcons::Avatar,624,185,52,White);
    if(social_.controller>=0)c.Box(668,226,12,12,ink,6);
    c.TextFit(697,181,social_.controller>=0?social_.displayName:"Perfil Xbox",25,White,315);
    const char* state=social_.origin==Origin::Test?"PERFIL DE TESTE":social_.state==SignIn::Online?"Sessão online":social_.state==SignIn::Local?"Sessão local":social_.state==SignIn::SignedOut?"Sem perfil":"Não consultado";
    c.TextFit(697,218,state,16,Muted,315);
    const std::string labels[]={
        social_.controller>=0?"Controle "+number(static_cast<size_t>(social_.controller+1)):"Controle: indisponível",
        social_.gamerscoreKnown?"Gamerscore: "+number(social_.gamerscore):"Gamerscore: indisponível",
        "Cartão do jogador","Mensagens / voz: pendentes","Amigos Xbox"
    };
    const int icons[]={1,9,27,19,8};
    for(int row=0;row<5;++row){
        const int y=274+row*42;
        const bool action=row==2||row==4;
        const bool selected=socialGuideFocus_&&((row==2&&socialGuideRow_==0)||(row==4&&socialGuideRow_==1));
        if(selected)c.SeriesTile(601,y-2,428,41,true,225);
        approvedIcon(c,icons[row],611,y+6,24,White);
        if(action){
            const auto kind=row==2?NativeAction::GamerCard:NativeAction::Friends;
            c.TextFit(649,y,labels[row],18,White,365);
            c.TextFit(649,y+21,CanRequestNative(kind,social_)?"Interface nativa experimental":"Requer perfil compatível no console",12,Muted,365);
        }else c.TextFit(649,y+3,labels[row],18,row==0?White:Muted,365);
        if(row<4)c.Box(611,y+39,403,1,0xff414743u);
    }
    c.SeriesTile(601,500,428,57,false,215);
    InfinityButtons::Draw(c,InfinityButtons::LB,616,513,30);
    c.TextFit(678,506,"Selecionar perfil Xbox",19,White,336);
#if defined(__XSDK__)
    c.TextFit(678,532,"Interface oficial do sistema.",13,Muted,336);
#else
    c.TextFit(678,532,"Interface oficial apenas no console.",13,Muted,336);
#endif
    c.SeriesTile(601,565,428,57,false,215);
    InfinityButtons::Draw(c,InfinityButtons::RB,616,578,30);
    c.TextFit(678,571,"Conquistas",19,White,336);
    c.TextFit(678,597,"Perfil ativo e copia GPD offline.",13,Muted,336);
}
void ConsoleHome::DrawQuickGuide(HomeCanvas& c)const{
    const auto& order=quickGuideEditing_?quickGuideDraft_:quickGuideOrder_;
    c.DimBackdrop();
    c.SeriesTile(235,87,332,552,false,244);
    approvedIcon(c,24,260,111,30,White);
    c.TextFit(301,105,c.Tr(quickGuideEditing_?"Personalizar guia":"Guia rápido"),28,White,250);
    const int icons[]={23,20,21,3,22,5,10,4};
    for(int i=0;i<QuickGuide::Count;++i){
        const unsigned action=static_cast<unsigned>(order[i]-'0');
        const int y=165+i*46;
        const bool focused=i==quickGuideRow_&&(!socialGuideFocus_||quickGuideEditing_);
        c.SeriesTile(255,y,292,41,focused,focused?225:190);
        approvedIcon(c,icons[action],269,y+9,23,White);
        c.TextFit(306,y+6,c.Tr(QuickGuide::Label(action)),18,White,207);
        if(action==QuickGuide::Notifications&&notifications_.UnreadCount())
            c.TextFit(509,y+7,number(notifications_.UnreadCount()),15,Lime,34);
        if(action==QuickGuide::Transfers&&TransferRunning())
            c.TextFit(509,y+7,number(transfer_.Percent())+"%",15,Lime,34);
    }
    c.Box(255,545,292,1,0xff414743u);
    if(quickGuideEditing_)InfinityButtons::Strip(c,259,551,"LB/RB Mover | X Padrão | Y Salvar",13,White,284);
    else {InfinityButtons::Draw(c,InfinityButtons::A,260,561,20);c.Text(286,558,"Abrir",15,White,59);InfinityButtons::Draw(c,InfinityButtons::X,350,561,20);c.Text(376,558,"Editar",15,White,58);InfinityButtons::Draw(c,InfinityButtons::B,439,561,20);c.Text(465,558,"Fechar",15,White,78);}
    c.TextFit(259,588,c.Tr(quickGuideEditing_?"B Cancelar edição":socialGuideFocus_?"Esquerda: atalhos | A Abrir":"Direita: perfil e amigos"),15,Muted,284);
    if(!quickGuideEditing_)DrawSocialGuide(c);
    if(!notice_.empty()){c.SeriesTile(580,644,470,36,false,250);c.TextFit(592,648,c.Tr(notice_.c_str()),14,White,446);}
}
void ConsoleHome::Draw(HomeCanvas& c,const std::string& clock,int64_t now) const {
    if(artDirty_) {c.ClearLocalArt();artDirty_=false;}
    if(wallpaperDirty_){c.ClearWallpaper();wallpaperDirty_=false;}
    c.SetTheme(appearanceEditor_?themeDraft_:theme_);c.SetLanguage(language_);
    c.SetAppearance(appearanceEditor_?appearanceDraft_:appearance_,FileWorkspace::Join(FileWorkspace::Join(dir_,"UserFiles"),"wallpaper.rgba"));
    c.Background();
    const bool approved=ApprovedHeader()&&!weatherPanel_;
    c.SeriesBackground();
    DrawApprovedHeader(c,approved&&focus_==ApprovedHome::Weather);
    const int64_t stamp=now<0?static_cast<int64_t>(std::time(nullptr)):now;
    c.SeriesTile(1046,12,206,52,false);
    c.TextFit(1058,18,clockSettings_.offset==ClockWeather::Auto?clock:ClockLabel(stamp),26,White,97);
    c.Box(1159,20,1,35,0xff65736du);
    c.TextFit(1166,16,ClockWeather::Date(stamp,clockSettings_.offset),13,White,79);
    c.TextFit(1166,38,ClockWeather::Zone(clockSettings_.offset),12,Muted,79);
    if(page_!=Home && section_==Iptv) {
        tv_.Draw(c,stamp,true);
        return;
    }
    if(page_==Home) DrawHome(c); else DrawSection(c);
    if(approvedGroup_>=0){DrawApprovedGroup(c);return;}
    if(quickGuideOpen_){DrawQuickGuide(c);return;}
    if(TransferRunning() && !(section_==System && page_==Details && systemTab_==3)) {
        c.Text(690,71,(copyPaused_?std::string("Fila pausada"):std::string("Copia ")+number(transfer_.Percent())+"%")+
               " | "+number(copyQueue_.size())+" aguardando",16,Lime,540);
    }
    if(!notice_.empty()) {
        c.Box(40,628,1200,35,0xff102923u,6);
        c.Text(51,630,c.Tr(notice_.c_str()),18,Lime,1170);
    }
    c.Translucent(0,663,1280,57,0xff0b110eu,180);c.Box(28,663,1224,1,0xff414743u);
    // Modals and specialised pages must not inherit unrelated Home commands.
    if(weatherPanel_){InfinityButtons::Strip(c,45,674,"Y Atualizar | B Voltar",19,White,1190);return;}
    if(storeActive_){
        const char* commands=storeProviders_?"A Abrir fonte | B Voltar | X Todos | Y Recarregar":storeManage_?"A Preparar | B Voltar":storeConfirm_?"A Confirmar | B Cancelar":storeDetails_?"A Pedido PC | B Voltar | X Remover PC | START Baixar":"A Detalhes | B Voltar | Y Atualizar | R3 Fontes";
        InfinityButtons::Strip(c,45,674,commands,19,White,1190);return;
    }
    if(homeEditing_||appearanceEditor_){InfinityButtons::Strip(c,45,674,"X Restaurar | Y Salvar | B Cancelar",19,White,1190);return;}
    if(importPreview_){InfinityButtons::Strip(c,45,674,"A Importar | B Cancelar",19,White,1190);return;}
    if(page_==Details&&section_==System){InfinityButtons::Strip(c,45,674,"B Voltar",19,White,1190);return;}
    if(contentActive_||artEditing_||discSetActive_||updateActive_||fileAction_||collectionPicker_||searchActive_){InfinityButtons::Strip(c,45,674,"B Voltar",19,White,1190);return;}
    buttonHint(c,45,"A",importPreview_?"Importar":section_==System && page_==Details && systemTab_==3 && TransferRunning()?(copyPaused_?"Retomar":"Pausar"):"Selecionar",0xff428914u);
    if(!approved||ApprovedHome::CoverSlot(focus_))buttonHint(c,254,"B",approved?"Girar capa":"Voltar",0xffb83c28u);
    if(!approved||ApprovedHome::CoverSlot(focus_)||focus_==ApprovedHome::Hero)buttonHint(c,429,"X",approved&&ApprovedHome::CoverSlot(focus_)?"Favorito":approved&&focus_==ApprovedHome::Hero?"Destaque":page_==Home && !homeEditing_ && !importPreview_?"Importar":(artEditing_ || (page_==Home && homeEditing_))?"Restaurar":section_==System && page_==Details && systemTab_==3?"Cancelar":
        section_==Files && page_!=Home && !searchActive_?"Copiar":"Favorito",0xff117db1u);
    buttonHint(c,632,"Y",(artEditing_ || (page_==Home && homeEditing_))?"Salvar":"Atualizar",0xffb3820du);
    if(approved){const auto storage=std::find_if(storageVolumes_.begin(),storageVolumes_.end(),[](const StorageVolume& v){return v.available;});if(storage!=storageVolumes_.end())c.TextFit(850,679,StorageInfo::FormatBytes(storage->freeBytes)+" livres",16,White,390);}
}

#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMB_MENU)
namespace {
int smbConsoleEntropy(void* buffer,size_t size){
    auto* p=static_cast<unsigned char*>(buffer);
    while(size){
        const size_t n=size>256?256:size;
        if(getentropy(p,n)!=0)return errno?errno:EIO;
        // Detect the known constant-byte emulator entropy stub; not a general
        // entropy quality test. Real credentials must never be used in Xenia.
        if(n>=8){bool same=true;for(size_t i=1;i<n;++i)if(p[i]!=p[0])same=false;if(same)return EIO;}
        p+=n;size-=n;
    }
    return 0;
}
}
bool ConsoleHome::SmbEnabled(bool writes)const{
    std::ifstream in((dir_+"enable-smb-session.txt").c_str());std::string token;
    if(!std::getline(in,token)||token!="ENABLE-SMB-SESSION")return false;
    if(!writes)return true;
    std::ifstream out((dir_+"enable-file-writes.txt").c_str());
    return std::getline(out,token)&&token=="ENABLE-FILE-WRITES";
}
void ConsoleHome::WipeSmbCredentials(){
    for(auto& field:smbCredentials_){
        volatile char* p=field.empty()?nullptr:&field[0];
        for(size_t i=0;i<field.size();++i)p[i]=0;
        field.clear();
    }
}
void ConsoleHome::CloseSmbSession(){
    smbSession_.Cancel();smbStage_.Cancel();WipeSmbCredentials();
    smbBrowser_.Reset();smbDestinations_.clear();smbPlan_={};smbConfirm_=false;
    smbUi_=SmbClosed;smbMessage_.clear();
}
bool ConsoleHome::TickSmbSession(){
    if(smbUi_==SmbClosed||smbUi_==SmbCredentials||smbUi_==SmbError)return false;
    if(!SmbEnabled(smbUi_==SmbCopying)){
        CloseSmbSession();smbUi_=SmbError;smbMessage_="Permissao SMB ou escrita removida.";return true;
    }
    const bool changed=smbSession_.Tick();
    if(smbSession_.Status()==SmbReadSession::Failed){
        const auto reason=smbSession_.Label();CloseSmbSession();smbUi_=SmbError;smbMessage_=reason;return true;
    }
    if(smbUi_==SmbConnecting&&smbSession_.Status()==SmbReadSession::Ready){
        if(!smbBrowser_.BeginRoot()||!smbSession_.Open("")){
            CloseSmbSession();smbUi_=SmbError;smbMessage_="Falha ao consultar a raiz.";return true;
        }
        smbUi_=SmbBrowsing;return true;
    }
    if(smbUi_==SmbBrowsing&&smbBrowser_.Pending()&&smbSession_.Status()==SmbReadSession::Ready){
        std::vector<SmbBrowser::Entry> entries;
        for(const auto& e:smbSession_.Entries())entries.push_back({e.name,e.size,e.directory});
        smbBrowser_.Commit(std::move(entries),smbSession_.Truncated());return true;
    }
    if(smbUi_==SmbCopying&&smbSession_.CopyComplete()){
        const bool ok=SmbEnabled(true)&&smbStage_.Publish(fileWorkspace_);
        smbStage_.Cancel();smbUi_=SmbBrowsing;
        smbMessage_=ok?"Copia publicada no workspace.":"Copia nao publicada; destino ocupado ou indisponivel.";
        return true;
    }
    return changed||smbUi_==SmbCopying;
}
void ConsoleHome::PressSmbSession(Key key){
    if(key==Back){
        if(smbUi_==SmbDestination){smbUi_=SmbBrowsing;smbConfirm_=false;return;}
        if(smbUi_==SmbBrowsing&&!smbBrowser_.Pending()&&smbBrowser_.BeginParent()){
            if(!smbSession_.Open(smbBrowser_.Requested()))smbBrowser_.Abort();
            return;
        }
        CloseSmbSession();return;
    }
    if(smbUi_==SmbError)return;
    if(smbUi_==SmbCredentials){
        auto& field=smbCredentials_[smbField_];
        const size_t limits[]={80,128,128,256};
        // 95 printable ASCII characters, case and punctuation preserved.
        if(key==Left)smbChar_=(smbChar_+94)%95;
        if(key==Right)smbChar_=(smbChar_+1)%95;
        if(key==Up)smbChar_=(smbChar_+85)%95;
        if(key==Down)smbChar_=(smbChar_+10)%95;
        if(key==PageUp)smbField_=(smbField_+3)%4;
        if(key==PageDown)smbField_=(smbField_+1)%4;
        if(key==Accept&&field.size()<limits[smbField_])field+=char(32+smbChar_);
        if(key==Favorite&&!field.empty()){field.back()=0;field.pop_back();}
        if(key==Refresh){
            LanPeerConfig::Octets ip{};
            if(!SmbEnabled()||!LanPeerConfig::Read(dir_+"infinity360-smb-peer.txt",ip)){
                smbMessage_="Configure IPv4 e habilite sessao primeiro.";return;
            }
            RefreshNetwork();
            const auto server=LanPeerConfig::Format(ip);
            if(!LanPeerProbe::Private(network_.ipv4)||server==network_.ipv4){
                smbMessage_="IPv4 local ou servidor invalido.";return;
            }
            smb2_set_random_provider(smbConsoleEntropy);
            const bool ok=smbSession_.Start(server,smbCredentials_[0],smbCredentials_[1],smbCredentials_[2],smbCredentials_[3]);
            WipeSmbCredentials();
            smbUi_=ok?SmbConnecting:SmbError;smbMessage_=ok?"":smbSession_.Label();
        }
        return;
    }
    if(smbUi_==SmbConnecting||smbUi_==SmbCopying){if(key==Favorite)CloseSmbSession();return;}
    if(smbUi_==SmbBrowsing){
        if(smbBrowser_.Pending())return;
        const auto n=smbBrowser_.Rows().size(),row=smbBrowser_.Row();
        if(key==Up&&row>0)smbBrowser_.Select(row-1);
        if(key==Down&&row+1<n)smbBrowser_.Select(row+1);
        if(key==PageUp)smbBrowser_.Select(row>=4?row-4:0);
        if(key==PageDown&&n)smbBrowser_.Select(std::min(n-1,row+4));
        if(key==Refresh&&smbBrowser_.BeginRefresh()){
            if(!smbSession_.Open(smbBrowser_.Requested()))smbBrowser_.Abort();
        }
        if(key==Accept&&row<n){
            if(smbBrowser_.Rows()[row].directory){
                if(smbBrowser_.BeginChild()&&!smbSession_.Open(smbBrowser_.Requested()))smbBrowser_.Abort();
            }else if(!SmbEnabled(true)||!smbBrowser_.Plan(smbPlan_)){
                smbMessage_="Escrita desabilitada ou arquivo incompatível com FATX.";
            }else{
                smbDestinations_.clear();
                for(const auto& root:fileWorkspace_.Roots()){
                    if(root!="hdd:/"&&root!="usb0:/"&&root!="usb1:/"&&root!="usb2:/")continue;
                    if(fileWorkspace_.CanWrite(fileWorkspace_.WorkspaceFor(root)))smbDestinations_.push_back(root);
                }
                if(smbDestinations_.empty())smbMessage_="Crie Infinity360Files no HDD/USB autorizado primeiro.";
                else{smbDest_=0;smbConfirm_=false;smbUi_=SmbDestination;smbMessage_.clear();}
            }
        }
        return;
    }
    if(smbUi_==SmbDestination){
        if(key==Left&&smbDest_>0){--smbDest_;smbConfirm_=false;}
        if(key==Right&&smbDest_+1<smbDestinations_.size()){++smbDest_;smbConfirm_=false;}
        if(key==Accept){
            if(!smbConfirm_){smbConfirm_=true;return;}
            smbConfirm_=false;
            if(!SmbEnabled(true)||!smbStage_.Start(fileWorkspace_,smbDestinations_[smbDest_],smbPlan_)){
                smbStage_.Cancel();smbUi_=SmbBrowsing;smbMessage_="Destino ocupado, indisponivel ou sem permissao.";return;
            }
            if(!smbSession_.ReadFile(smbPlan_,[this](const uint8_t* data,size_t n){return smbStage_.Write(data,n);})){
                smbStage_.Cancel();smbUi_=SmbBrowsing;smbMessage_="Leitura remota nao iniciada.";return;
            }
            smbUi_=SmbCopying;smbMessage_.clear();
        }
    }
}
void ConsoleHome::DrawSmbSession(HomeCanvas& c)const{
    c.Text(45,349,std::string(c.Tr("SMB 3.1.1 | Sessao experimental")),27,White,1170);
    if(smbUi_==SmbCredentials){
        const char* labels[]={"Compartilhamento","Usuario","Dominio (opcional)","Senha"};
        for(unsigned i=0;i<4;++i){
            const int x=48+(i%2)*600,y=386+(i/2)*61;
            panel(c,x,y,582,55,Panel,i==smbField_);
            const std::string value=i==3?std::string(smbCredentials_[i].size(),'*'):smbCredentials_[i];
            c.Text(x+10,y+3,c.Tr(labels[i]),16,Muted,550);
            c.Text(x+10,y+24,value.empty()?"--":value,19,White,550);
        }
        c.Text(48,515,std::string(std::string(c.Tr("Caractere selecionado: [")))+char(32+smbChar_)+std::string(c.Tr("] | A adiciona | X apaga")),20,Lime,1170);
        c.Text(48,550,smbMessage_.empty()?std::string(c.Tr("Credenciais em RAM; use conta de teste. Nunca use senha real no emulador.")):c.Tr(smbMessage_.c_str()),17,Muted,1170);
        InfinityButtons::Strip(c,48,607,std::string(c.Tr("Direcoes Caractere | LB/RB Campo | Y Conectar | B Cancelar")),18,Lime,1170);return;
    }
    if(smbUi_==SmbBrowsing){
        c.Text(48,386,"/"+smbBrowser_.Current(),20,White,1170);
        if(smbBrowser_.Pending())c.Text(48,434,std::string(c.Tr("Consultando diretorio...")),22,Muted,1170);
        else{
            const auto& rows=smbBrowser_.Rows();const auto first=(smbBrowser_.Row()/4)*4;
            for(size_t i=first;i<rows.size()&&i<first+4;++i){
                const auto& e=rows[i];const int y=416+int(i-first)*33;
                panel(c,48,y,1184,30,Panel,i==smbBrowser_.Row());
                c.Text(57,y+3,(e.directory?"[DIR] ":"[FILE] ")+e.name,18,White,930);
                if(!e.directory)c.Text(1000,y+3,number(e.size)+" B",17,Muted,220);
            }
            if(rows.empty())c.Text(48,434,std::string(c.Tr("Diretorio vazio.")),22,Muted,1170);
        }
        c.Text(48,561,smbMessage_.empty()?(smbBrowser_.Truncated()?std::string(c.Tr("Listagem limitada a 512 itens.")):std::string(c.Tr("Somente leitura remota; arquivos copiados apenas em workspace."))):std::string(c.Tr(smbMessage_.c_str())),17,Muted,1170);
        InfinityButtons::Strip(c,48,607,std::string(c.Tr("A Abrir/Copiar | Y Atualizar | LB/RB Pagina | B Voltar")),18,Lime,1170);return;
    }
    if(smbUi_==SmbDestination){
        c.Text(48,395,smbPlan_.name+" | "+number(smbPlan_.bytes)+" B",22,White,1170);
        c.Text(48,447,std::string(c.Tr("Destino: "))+fileWorkspace_.WorkspaceFor(smbDestinations_[smbDest_]),22,Lime,1170);
        c.Text(48,503,smbConfirm_?std::string(c.Tr("A novamente confirma a copia sem sobrescrever.")):std::string(c.Tr("Esquerda/Direita escolhe HDD ou USB.")),20,Muted,1170);
        InfinityButtons::Strip(c,48,607,std::string(c.Tr("A Confirmar | B Cancelar")),18,Lime,1170);return;
    }
    if(smbUi_==SmbCopying){
        const auto done=smbSession_.Done(),total=smbSession_.Total();
        const unsigned percent=total?unsigned(done*100/total):0;
        c.Text(48,395,smbPlan_.name,23,White,1170);
        c.Box(48,450,1180,24,Panel,5);c.Box(48,450,int(1180*percent/100),24,Lime,5);
        c.Text(48,488,number(done)+" / "+number(total)+" B | "+number(percent)+"%",21,White,1170);
        c.Text(48,549,c.Tr(smbSession_.Label().c_str()),18,Muted,1170);
        c.Text(48,607,std::string(c.Tr("X/B Cancelar sessao e remover parcial")),18,Lime,1170);return;
    }
    c.Text(48,418,c.Tr((smbUi_==SmbError?smbMessage_:smbSession_.Label()).c_str()),23,White,1170);
    c.Text(48,607,std::string(c.Tr("B Voltar")),18,Lime,1170);
}
#endif

void ConsoleHome::RefreshSaveProfiles(){
    saveProfiles_=ContentCatalog(ContentCatalog::DefaultRoots(dir_)).SaveProfiles(saveLimited_);
    saveProfileRow_=saveRow_=saveBackupRow_=0;saveList_=saveBackups_=saveConfirm_=false;
    saveItems_.clear();saveRecords_.clear();saveTitle_=0;
}
void ConsoleHome::RefreshSaveItems(){
    saveItems_.clear();saveRow_=0;saveLimited_=false;saveConfirm_=false;saveBackups_=false;
    if(saveProfileRow_>=saveProfiles_.size())return;
    const auto index=ContentCatalog(ContentCatalog::DefaultRoots(dir_)).ScanProfileSaves(saveProfiles_[saveProfileRow_]);
    saveItems_=index.saves;saveLimited_=index.truncated;
}
void ConsoleHome::RefreshSaveRecords(){
    saveRecords_.clear();
    for(const auto& rec:ContentBackup::List(fileWorkspace_,dir_,saveTitle_))
        if(rec.package.contentType==1)saveRecords_.push_back(rec);
    if(saveBackupRow_>=saveRecords_.size())saveBackupRow_=0;
}
void ConsoleHome::PressSaveCenter(Key key){
    if(backupPending_||recoverPending_)return;
    if(saveConfirm_){
        saveConfirm_=false;
        if(key!=Accept){notice_="Operacao cancelada.";return;}
#if defined(INFINITY_XSDK_HTTPS)
        if(PowerBusy()){notice_="Encerre FTP e demais operacoes.";return;}
        if(saveBackups_)recoverPending_=true;else backupPending_=true;
        notice_="Operacao solicitada.";
#else
        notice_="Provider de backup indisponivel nesta build.";
#endif
        return;
    }
    if(key==Back){
        if(saveBackups_){saveBackups_=false;saveRecords_.clear();}
        else if(saveList_){saveList_=false;saveItems_.clear();}
        else saveCenter_=false;
        notice_.clear();return;
    }
    if(key==Refresh){
        if(saveBackups_)RefreshSaveRecords();
        else if(saveList_)RefreshSaveItems();
        else RefreshSaveProfiles();
        notice_.clear();return;
    }
    auto& row=saveBackups_?saveBackupRow_:saveList_?saveRow_:saveProfileRow_;
    const size_t count=saveBackups_?saveRecords_.size():saveList_?saveItems_.size():saveProfiles_.size();
    if(key==Up&&row>0)--row;
    if(key==Down&&row+1<count)++row;
    if(key==PageUp)row=row>=4?row-4:0;
    if(key==PageDown&&count)row=std::min(count-1,row+4);
    if(row>=count)return;
    if(!saveList_&&key==Accept){saveList_=true;RefreshSaveItems();notice_.clear();return;}
    if(saveList_&&!saveBackups_&&key==Guide){
        saveTitle_=saveItems_[row].titleId;saveBackupRow_=0;saveBackups_=true;RefreshSaveRecords();notice_.clear();return;
    }
    if(saveList_&&(key==Accept||(saveBackups_&&key==Favorite))){
        if(saveBackups_){recoverItem_=saveRecords_[row];verifyOnly_=key==Favorite;}
        else {backupItem_=saveItems_[row];saveTitle_=backupItem_.titleId;}
        saveConfirm_=true;
        notice_=saveBackups_?(verifyOnly_?"A confirma verificar SHA-256.":"A confirma recuperar em UserFiles, sem alterar save ativo."):"A confirma backup do save selecionado.";
    }
}
void ConsoleHome::DrawSaveCenter(HomeCanvas& c)const{
    c.Text(45,349,saveBackups_?std::string(c.Tr("Saves | Backups do Title ID "))+hexId(saveTitle_):saveList_?std::string(c.Tr("Saves | "))+saveProfiles_[saveProfileRow_].owner+" / "+saveProfiles_[saveProfileRow_].device:std::string(c.Tr("Perfis e saves | Inventario de armazenamento")),25,White,1170);
    const size_t row=saveBackups_?saveBackupRow_:saveList_?saveRow_:saveProfileRow_;
    const size_t count=saveBackups_?saveRecords_.size():saveList_?saveItems_.size():saveProfiles_.size();
    const size_t first=(row/4)*4;
    for(size_t i=first;i<count&&i<first+4;++i){
        const int y=389+int(i-first)*40;panel(c,48,y,1184,36,Panel,i==row);
        std::string label;
        if(saveBackups_)label=saveRecords_[i].slot+" | "+saveRecords_[i].package.name+" | "+number(saveRecords_[i].bytes)+" B";
        else if(saveList_)label=hexId(saveItems_[i].titleId)+" | "+saveItems_[i].name+" | "+saveItems_[i].path.substr(saveItems_[i].path.find_last_of("/\\")+1);
        else label=saveProfiles_[i].owner+" | "+saveProfiles_[i].device+(saveProfiles_[i].owner=="0000000000000000"?std::string(c.Tr(" | Compartilhado")):std::string(c.Tr(" | Pasta de perfil")));
        InfinityIcons::Draw(c,saveBackups_?39:saveList_?38:27,58,y+6,23,White);
        c.TextFit(94,y+6,label,19,White,1100);
    }
    if(!count)c.Text(48,410,std::string(c.Tr("Nenhum registro reconhecido.")),22,Muted,1170);
    c.Text(48,555,notice_.empty()?(saveBackups_?std::string(c.Tr("Backups por Title ID, podem pertencer a outros perfis.")):std::string(c.Tr("Inventario por pasta, nao comprova titularidade. "))+std::string(saveLimited_?std::string(c.Tr("Limite atingido.")):"")):std::string(c.Tr(notice_.c_str())),17,Muted,1170);
    InfinityButtons::Strip(c,48,607,saveConfirm_?std::string(c.Tr("A Confirmar | B Cancelar")):saveBackups_?std::string(c.Tr("A Recuperar copia | X Verificar | Y Atualizar | B Voltar")):saveList_?std::string(c.Tr("A Backup | START Backups do jogo | Y Atualizar | B Voltar")):std::string(c.Tr("A Saves | Y Atualizar | LB/RB Pagina | B Voltar")),18,Lime,1170);
}

void ConsoleHome::RefreshAchievements(){
    achievementSummary_={};achievementItems_.clear();achievementRow_=achievementDetailRow_=0;
    achievementDetails_=false;achievementLoaded_=false;notice_.clear();
    const auto path=FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(dir_,"UserFiles"),"Achievements"),"FFFE07D1.gpd");
    std::vector<uint8_t> data;
    if(fileWorkspace_.CanReadFile(path)&&GpdAchievements::Read(path,data)&&GpdAchievements::Dashboard(data,achievementSummary_)){
        achievementLoaded_=true;
        notice_="Copia GPD local; dados nao autenticados nem sincronizados.";
    }else notice_="Importe FFFE07D1.gpd em UserFiles/Achievements.";
}
void ConsoleHome::PressAchievements(Key key){
    if(key==Back){
        if(achievementDetails_){achievementDetails_=false;achievementItems_.clear();notice_.clear();}
        else{achievementsPage_=false;achievementLoaded_=false;achievementSummary_={};achievementItems_.clear();notice_.clear();}
        return;
    }
    if(key==Refresh){RefreshAchievements();return;}
    auto& row=achievementDetails_?achievementDetailRow_:achievementRow_;
    const size_t count=achievementDetails_?achievementItems_.size():achievementSummary_.titles.size();
    if(key==Up&&row>0)--row;
    if(key==Down&&row+1<count)++row;
    if(key==PageUp)row=row>=4?row-4:0;
    if(key==PageDown&&count)row=std::min(count-1,row+4);
    if(key==Accept&&!achievementDetails_&&row<count){
        const auto title=achievementSummary_.titles[row].id;
        const auto path=FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(dir_,"UserFiles"),"Achievements"),hexId(title)+".gpd");
        std::vector<uint8_t> data;
        achievementItems_.clear();
        if(!fileWorkspace_.CanReadFile(path)||!GpdAchievements::Read(path,data)||!GpdAchievements::Game(data,achievementItems_)){
            notice_="GPD do titulo ausente ou invalido: "+hexId(title)+".gpd";return;
        }
        achievementDetails_=true;achievementDetailRow_=0;
        notice_="GPD associado pelo nome do arquivo; perfil nao autenticado.";
    }
}
void ConsoleHome::DrawAchievements(HomeCanvas& c)const{
    c.Text(45,349,achievementDetails_?std::string(c.Tr("Conquistas | "))+achievementSummary_.titles[achievementRow_].name:std::string(c.Tr("Conquistas Xbox | GPD offline")),26,White,1170);
    if(achievementLoaded_&&!achievementDetails_){
        const auto& s=achievementSummary_;
        c.TextFit(48,382,"GPD offline: G "+number(s.earnedScore)+"/"+number(s.score)+std::string(c.Tr(" | Conquistas "))+number(s.earned)+"/"+number(s.total)+std::string(c.Tr(" | Titulos "))+number(s.titles.size()),19,Lime,1170);
    }
    const size_t row=achievementDetails_?achievementDetailRow_:achievementRow_;
    const size_t count=achievementDetails_?achievementItems_.size():achievementSummary_.titles.size();
    const size_t first=(row/4)*4;
    for(size_t i=first;i<count&&i<first+4;++i){
        const int y=412+int(i-first)*32;panel(c,48,y,1184,30,Panel,i==row);
        std::string label;
        if(achievementDetails_){const auto& a=achievementItems_[i];label=std::string(a.earned?"[OK] ":"[--] ")+a.name+" | "+number(a.score)+" G";}
        else{const auto& t=achievementSummary_.titles[i];label=hexId(t.id)+" | "+t.name+" | "+number(t.earned)+"/"+number(t.total)+" | "+number(t.earnedScore)+"/"+number(t.score)+" G";}
        InfinityIcons::Draw(c,9,58,y+4,22,White);c.TextFit(90,y+4,label,18,White,1110);
    }
    if(achievementLoaded_&&!count)c.Text(48,432,std::string(c.Tr("Nenhuma entrada de conquista reconhecida.")),21,Muted,1170);
    const std::string active=social_.controller<0?"Perfil ativo: indisponivel":
        (social_.origin==InfinitySocial::Origin::Test?"Perfil de teste: ":"Perfil ativo: ")+social_.displayName+
        (social_.gamerscoreKnown?" | G reportado: "+number(social_.gamerscore):" | G indisponivel");
    c.TextFit(48,541,active+" | GPD nao vinculado ao perfil",16,White,1170);
    c.TextFit(48,568,notice_.empty()?std::string(c.Tr("Dados locais, nao verificados pela Xbox Live.")):std::string(c.Tr(notice_.c_str())),17,Muted,1170);
    InfinityButtons::Strip(c,48,607,achievementDetails_?std::string(c.Tr("LB/RB Pagina | Y Recarregar resumo | B Voltar")):std::string(c.Tr("A Detalhes | LB/RB Pagina | Y Importar novamente | B Voltar")),18,Lime,1170);
}

void ConsoleHome::PressAppearance(Key key){
    if(layoutPreview_){if(key==Back||key==Search)layoutPreview_=false;return;}
    if(key==Back){appearanceEditor_=false;notice_.clear();return;}
    if(key==Search){layoutPreview_=layoutDraft_!=0;return;}
    if(key==Up)appearanceRow_=(appearanceRow_+7)%8;
    if(key==Down)appearanceRow_=(appearanceRow_+1)%8;
    if(key==Left||key==Right||key==Accept){
        auto cycle=[&](unsigned& value,unsigned count){value=(value+(key==Left?count-1:1))%count;};
        if(appearanceRow_==0)cycle(themeDraft_,HomeCanvas::ThemeCount);
        if(appearanceRow_==1)cycle(appearanceDraft_.background,AppearanceSettings::BackgroundCount);
        if(appearanceRow_==2)cycle(appearanceDraft_.dim,AppearanceSettings::DimCount);
        if(appearanceRow_==3)cycle(appearanceDraft_.artwork,AppearanceSettings::ArtworkCount);
        if(appearanceRow_==4)cycle(layoutDraft_,DashboardLayout::Count);
        if(appearanceRow_==5)cycle(gridDraft_,GameGrid::Count);
        if(appearanceRow_==6)clockDraft_.offset=ClockWeather::NextOffset(clockDraft_.offset,key==Left?-1:1);
        if(appearanceRow_==7)cycle(clockDraft_.city,ClockWeather::CityCount);
    }
    if(key==Favorite){appearanceDraft_={};clockDraft_={};themeDraft_=0;layoutDraft_=initialLayout_;gridDraft_=0;notice_.clear();}
    if(key==Guide){wallpaperDirty_=true;notice_="Papel de parede recarregado na proxima renderizacao.";}
    if(key==Refresh){
        const auto previous=appearance_;const auto previousClock=clockSettings_;const auto previousTheme=theme_,previousLayout=layout_,previousGrid=gridPreference_;
        appearance_=appearanceDraft_;clockSettings_=clockDraft_;theme_=themeDraft_;layout_=layoutDraft_;gridPreference_=gridDraft_;gridFilter_=false;
        if(SaveProfile()){appearanceEditor_=false;focus_=0;if(previousClock.city!=clockSettings_.city)weather_={};if(clockSettings_.city)weatherPending_=true;notice_="Configuracoes salvas.";}
        else{appearance_=previous;clockSettings_=previousClock;theme_=previousTheme;layout_=previousLayout;gridPreference_=previousGrid;notice_="Falha ao salvar configuracoes.";}
    }
}
void ConsoleHome::DrawAppearance(HomeCanvas& c)const{
    c.Text(45,349,c.Tr("Personalizar aparencia"),27,White,1170);
    const char* labels[]={"Paleta","Fundo da interface","Escurecimento","Arte dos jogos","Layout","Biblioteca","Fuso horario / UTC","Cidade para clima"};
    const std::string values[]={c.Tr(HomeCanvas::ThemeName(themeDraft_)),c.Tr(AppearanceSettings::BackgroundName(appearanceDraft_.background)),number(AppearanceSettings::Darkness(appearanceDraft_.dim))+"%",c.Tr(AppearanceSettings::ArtworkName(appearanceDraft_.artwork)),DashboardLayout::Name(layoutDraft_),c.Tr(GameGrid::Name(gridDraft_)),ClockWeather::Zone(clockDraft_.offset),ClockWeather::Cities[clockDraft_.city].name};
    for(unsigned i=0;i<8;++i){
        const int x=48+(i%2)*600,y=385+(i/2)*54;
        panel(c,x,y,582,49,Panel,i==appearanceRow_);
        const int icons[]={10,62,46,62,24,23,20,25};InfinityIcons::Draw(c,icons[i],x+12,y+10,28,White);
        c.TextFit(x+52,y+4,c.Tr(labels[i]),17,Muted,510);
        c.TextFit(x+52,y+21,"< "+values[i]+" >",20,Lime,510);
    }
    c.Text(48,596,"Clima: Open-Meteo | Cidade manual | UTC nao altera o relogio do sistema",16,Muted,1170);
    InfinityButtons::Strip(c,48,618,c.Tr("Direcoes Ajustar | R3 Previa | X Padrao | Y Salvar | B Cancelar"),17,Lime,1170);
}

void ConsoleHome::DrawModernHome(HomeCanvas& c,unsigned style,bool preview)const{
    if(style==DashboardLayout::TopIcons){DrawApprovedHome(c,preview);return;}
    const auto g=DashboardLayout::Get(style);
    const auto& appearance=preview?appearanceDraft_:appearance_;
    for(int i=0;i<DashboardLayout::Slots;++i){
        const auto rect=g.nav[i];const int destination=i>=Apps&&i<=System?Apps+homeOrder_[i-Apps]-'0':i;
        const bool active=i==(preview?0:focus_);
        const char* name=destination==8?"Loja":destination==9?"Downloads":destination==Library?"Jogos":destination==Iptv?"IPTV":labels[destination];
        panel(c,rect.x,rect.y,rect.w,rect.h,Panel,active);
        const int icons[]={1,3,2,1,14,10,20,21,4,5};
        const int icon=destination>=0&&destination<10?icons[destination]:0;
        const int size=rect.h>=70?30:24;
        const int ix=rect.w<=120?rect.x+(rect.w-size)/2:rect.x+12;
        const int iy=rect.w<=120?rect.y+8:rect.y+(rect.h-size)/2;
        InfinityIcons::Draw(c,icon,ix,iy,size,White);
        const int tx=rect.w<=120?rect.x+5:rect.x+size+22;
        const int ty=rect.w<=120?rect.y+43:rect.y+(rect.h-22)/2;
        c.Text(tx,ty,c.Tr(name),rect.w<=120?15:20,active?Lime:White,rect.x+rect.w-tx-5);
    }
    const auto hero=g.hero;panel(c,hero.x,hero.y,hero.w,hero.h,0xff102923u,false);
    const Game* featured=nullptr;
    for(const auto& path:history_){const auto* item=Installed(path);if(item&&!contains(hidden_,path)){featured=item;break;}}
    if(!featured)for(const auto& item:games_)if(!contains(hidden_,item.executable)){featured=&item;break;}
    const bool performance=style==DashboardLayout::Performance;
    const bool media=style==DashboardLayout::MediaHub;
    if(featured&&!performance&&!media){
        if(appearance.ShowBackdrop()){
            const auto file=ArtPath(*featured,false);
            if(!file.empty())c.LocalArt(file,false,hero.x+hero.w/2,hero.y+2,hero.w/2-2,hero.h-4);
        }
        c.Box(hero.x+10,hero.y+10,hero.w/2-16,hero.h-20,Panel,8);
        c.Text(hero.x+25,hero.y+26,c.Tr("Destaque"),21,Lime,hero.w/2-46);
        c.Text(hero.x+25,hero.y+70,featured->name,34,White,hero.w/2-46);
        c.Text(hero.x+25,hero.y+122,c.Tr("Abra Jogos para selecionar o titulo."),19,Muted,hero.w/2-46);
    }else{
        c.Text(hero.x+20,hero.y+20,performance?c.Tr("Indicadores locais"):media?"Cloud360 IPTV":"INFINITY 360",32,White,hero.w-40);
        c.Text(hero.x+20,hero.y+80,number(games_.size())+" "+c.Tr("Jogos")+" | "+number(tv_.Count())+" IPTV",22,Lime,hero.w-40);
        c.Text(hero.x+20,hero.y+120,number(copyQueue_.size())+" "+c.Tr("Downloads")+" / "+c.Tr("Tarefas locais"),20,Muted,hero.w-40);
        c.Text(hero.x+20,hero.y+160,performance?c.Tr("Sensores e rede: consultar em Sistema."):c.Tr("Selecione uma secao pelo menu."),19,Muted,hero.w-40);
    }
    const auto shelf=g.shelf;
    c.Text(shelf.x,shelf.y,c.Tr(style==DashboardLayout::Arcade?"Biblioteca":"Recentes"),22,White,shelf.w);
    const int cols=std::max(2,std::min(8,shelf.w/130)),cw=(shelf.w-(cols-1)*8)/cols;
    const int height=shelf.h-32;
    int shown=0;
    auto card=[&](const Game& game){
        if(shown>=cols||contains(hidden_,game.executable))return;
        const int x=shelf.x+shown*(cw+8),y=shelf.y+30;
        panel(c,x,y,cw,height,Panel,false);
        const auto cover=ArtPath(game,true),back=ArtPath(game,false);
        bool drawn=appearance.ShowCover()&&!cover.empty()&&c.LocalArt(cover,true,x+(cw-std::min(cw-8,(height-27)*2/3))/2,y+3,std::min(cw-8,(height-27)*2/3),height-27);
        if(!drawn&&appearance.ShowBackdrop()&&!back.empty())drawn=c.LocalArt(back,false,x+3,y+3,cw-6,height-27);
        if(!drawn)InfinityIcons::Draw(c,1,x+cw/2-15,y+10,30,Muted);
        c.Text(x+5,y+height-24,game.name,15,White,cw-10);++shown;
    };
    for(const auto& path:history_){if(shown>=cols)break;const auto* item=Installed(path);if(item)card(*item);}
    if(!shown)for(const auto& game:games_){if(shown>=cols)break;card(game);}
    if(!shown)c.Text(shelf.x+10,shelf.y+45,c.Tr("Nenhum item nesta seção."),20,Muted,shelf.w-20);
    if(preview){c.Box(44,628,1190,33,Panel,4);c.Text(55,632,c.Tr("Previa de layout | B Voltar ao editor"),18,Lime,1160);}
}

bool ConsoleHome::GridListing()const{
    return page_==Listing&&(section_==Library||section_==Recent||section_==Favorites)&&GameGrid::Enabled(gridPreference_,layout_);
}
void ConsoleHome::DrawGameGrid(HomeCanvas& c)const{
    const int count=static_cast<int>(Count());
    const int first=row_/GameGrid::Page*GameGrid::Page;
    const int top=section_==Library?239:202;
    const int cw=190,ch=174,stepX=200,stepY=182;
    if(count){
        for(int index=first;index<std::min(count,first+GameGrid::Page);++index){
            const auto& game=*visible_[index];const int slot=index-first;
            const int x=45+slot%GameGrid::Columns*stepX,y=top+slot/GameGrid::Columns*stepY;
            const bool active=index==row_&&!gridFilter_;
            panel(c,x,y,cw,ch,Panel,active);
            const auto cover=ArtPath(game,true),back=ArtPath(game,false);
            bool art=false;
            if(appearance_.ShowCover()&&!cover.empty())art=c.LocalArt(cover,true,x+2,y+2,cw-4,ch-4,true,9);
            if(!art&&appearance_.ShowBackdrop()&&!back.empty())art=c.LocalArt(back,false,x+2,y+2,cw-4,ch-4,true,9);
            if(art){for(int dy=0;dy<45;++dy)c.Translucent(x+5,y+ch-50+dy,cw-10,1,0xff090f0bu,unsigned(90+dy*3));}
            else{InfinityIcons::Draw(c,1,x+(cw-48)/2,y+46,48,Muted);c.CenterText(x+4,y+100,cw-8,"Sem capa",14,Muted);}
            if(Starred(game))InfinityIcons::Draw(c,21,x+9,y+9,19,Lime);
            if(contains(pinned_,game.executable))c.TextFit(x+cw-62,y+7,c.Tr("FIXADO"),12,Lime,54);
            c.TextFit(x+10,y+ch-32,game.name,17,White,cw-20);
            if(active)c.SeriesFocus(x,y,cw,ch);
        }
        c.Text(1060,597,number(first/GameGrid::Page+1)+"/"+number((count+GameGrid::Page-1)/GameGrid::Page),17,Lime,170);
    }
    const char* help=section_==Library?(gridFilter_?"Esq/Dir Categorias | A/baixo Jogos":"Direcoes Capas | Cima na 1a fila: categorias"):"Direcoes Capas | A Detalhes | X Favorito";
    c.Text(45,597,c.Tr(help),17,Muted,1000);
}

void ConsoleHome::DrawModernGameDetails(HomeCanvas& c)const{
    const auto& game=*visible_[row_];const auto& m=detailMetadata_;
    const auto cover=ArtPath(game,true),back=ArtPath(game,false);
    c.SeriesTile(45,145,1190,246,false);
    bool backdrop=false;
    if(appearance_.ShowBackdrop()&&!back.empty())backdrop=c.LocalArt(back,false,637,148,595,240,true,9);
    if(backdrop)c.Shade(640,151,130,234,0xff101715u,240,0);
    const bool art=appearance_.ShowCover()&&!cover.empty()&&c.LocalArt(cover,true,65,166,136,204,false,8);
    if(!art){c.SeriesTile(65,166,136,204,false);InfinityIcons::Draw(c,1,107,224,52,Muted);c.CenterText(68,289,130,"Sem capa",14,Muted);}
    c.TextFit(219,165,game.name,36,White,398);
    c.TextFit(219,220,game.device,18,Muted,398);
    c.TextFit(219,251,m.package?(m.volumeType==0?"STFS / GOD / XBLA":"SVOD"):"XEX",19,White,398);
    if(m.discCount)c.TextFit(219,277,std::string(c.Tr("Disco"))+" "+number(m.discNumber)+"/"+number(m.discCount),17,Muted,398);
    c.TextFit(219,305,contains(pinned_,game.executable)?c.Tr("FIXADO"):Starred(game)?c.Tr("Favorito"):"",15,Lime,398);
    c.Box(219,337,250,39,0xffa8ff00u,8);
    InfinityButtons::Draw(c,InfinityButtons::A,232,344,25);
    c.TextFit(266,340,c.Tr(m.package?"Solicitar montagem":"Jogar"),19,0xff101710u,194);
    const char* actions[]={"TU / DLC / Saves","Fixar / Desafixar","Capa e fundo","Favorito"};
    const int actionIcons[]={InfinityButtons::RB,InfinityButtons::LB,InfinityButtons::Start,InfinityButtons::X};
    for(int i=0;i<4;++i){
        const int x=45+i*300;c.SeriesTile(x,401,289,49,false);
        InfinityButtons::Draw(c,actionIcons[i],x+10,410,28);
        c.TextFit(x+58,413,c.Tr(actions[i]),16,White,220);
    }
    panel(c,45,461,590,125,Panel,false);
    panel(c,646,461,589,125,Panel,false);
    c.Text(57,467,c.Tr("Identidade do titulo"),19,Lime,565);
    if(m.known){
        c.Text(57,494,"Title "+hexId(m.titleId)+" | Media "+(m.mediaId?hexId(m.mediaId):"-"),18,White,565);
        c.Text(57,522,"XEX "+hexId(m.version)+" | "+(m.package?"Tipo "+hexId(m.contentType):"Base "+hexId(m.baseVersion)),17,Muted,565);
        if(m.package)c.Text(57,550,c.Tr("Tamanho declarado")+std::string(": ")+StorageInfo::FormatBytes(m.declaredBytes),16,Muted,565);
        else if(translationReferences_.count(m.titleId))c.Text(57,550,c.Tr("PT-BR referenciado; versao nao confirmada"),16,Muted,565);
    }else c.Text(57,501,c.Tr("Metadados indisponiveis para este item."),18,Muted,565);
    const auto stats=LaunchStats(game.executable);
    c.Text(658,467,c.Tr("Historico de execucao"),19,Lime,562);
    c.Text(658,495,c.Tr("Tentativas")+std::string(": ")+number(stats.attempts)+" | "+c.Tr("Falhas imediatas")+": "+number(stats.failed),17,White,562);
    c.Text(658,522,stats.attempts?LaunchStatistics::Date(stats.last):c.Tr("Nenhuma tentativa registrada"),17,Muted,562);
    c.Text(658,550,c.Tr("Y Atualizar"),17,Muted,562);
    c.Text(45,597,game.executable,16,Muted,1180);
    if(m.discCount>1)c.Text(45,619,c.Tr("R3 Selecionar disco"),17,Lime,1180);
}
void ConsoleHome::DrawModernStore(HomeCanvas& c)const{
    InfinityIcons::Draw(c,4,46,91,32,White);c.TextFit(91,78,"INFINITY Store",40,White,1128);
    c.Text(48,133,StoreSourceLabel()+" | "+StoreStorage::Label(store_.Storage()),20,Muted,1175);
    const char* tabs[]={"Todos","Jogos","XBLA","Homebrew","Apps","Emuladores","DLC","PT-BR"};
    for(int i=0;i<8;++i){
        const int x=45+i*149;panel(c,x,171,140,42,Panel,i==storeTab_);
        const int icons[]={2,1,35,42,2,1,37,45};InfinityIcons::Draw(c,icons[i],x+7,181,22,i==storeTab_?Lime:White);
        c.TextFit(x+34,180,c.Tr(tabs[i]),16,White,100);
    }
    if(storeVisible_.empty()){
        c.Text(55,270,c.Tr("Nenhum item no catalogo desta categoria."),23,White,1170);
        c.Text(55,320,c.Tr("Configure aurorax-store.tsv com fontes e licencas autorizadas."),19,Muted,1170);return;
    }
    const auto& item=store_.Items()[storeVisible_[storeRow_]];
    const auto status=[&](const StoreItem& p)->const char*{
        if(store_.Installed(p))return "Instalado";
        if(store_.Progress(p).state==StoreProgress::NativePaused)return "Parcial salvo";
        if(store_.Staged(p))return "Arquivo local";
        if(store_.NativeQueued(p))return "Na fila nativa";
        if(store_.Requested(p))return "Na fila PC";
        return "Disponivel";
    };
    if(!storeDetails_){
        const int first=storeRow_/5*5;
        for(int i=first;i<std::min(first+5,static_cast<int>(storeVisible_.size()));++i){
            const auto& p=store_.Items()[storeVisible_[i]];const int y=228+(i-first)*70;
            panel(c,45,y,675,64,Panel,i==storeRow_);
            c.Text(60,y+5,p.name,23,White,643);
            c.Text(60,y+34,p.version+" | "+StorageInfo::FormatBytes(p.bytes),17,Muted,643);
        }
    }
    const int x=storeDetails_?45:735,w=storeDetails_?1190:500;
    panel(c,x,228,w,344,Panel,false);
    c.SeriesTile(x+10,238,w-20,58,false,190);
    InfinityIcons::Draw(c,4,x+22,254,24,White);c.TextFit(x+58,249,item.name,26,White,w-78);
    c.Text(x+20,310,c.Tr(status(item)),21,Lime,w-40);
    c.Text(x+20,346,item.version+" | "+StorageInfo::FormatBytes(item.bytes),20,White,w-40);
    c.Text(x+20,379,std::string(c.Tr("Categoria"))+": "+item.category,18,Muted,w-40);
    c.Text(x+20,412,std::string(c.Tr("Licenca / autorizacao declarada"))+": "+item.rights,18,Muted,w-40);
    if(item.category=="translation"){
        c.Text(x+20,445,"Title "+hexId(item.titleId)+" | Media "+hexId(item.mediaId)+" | XEX "+hexId(item.gameVersion),17,Muted,w-40);
        if(storeDetails_)c.Text(x+20,479,c.Tr(storeTranslationMatch_?"Cabecalho local coincide; TU ativa nao validada":"Nenhum XEX local correspondente; pedido bloqueado"),17,storeTranslationMatch_?Lime:Muted,w-40);
    }else c.Text(x+20,445,item.provider.empty()?c.Tr("Catalogo local"):item.provider,18,Muted,w-40);
    if(storeDetails_){
        c.Text(x+20,515,item.url,16,Muted,w-40);
        InfinityButtons::Strip(c,45,581,c.Tr(storeConfirm_?"A Confirmar pedido | B Cancelar":"START Baixar | A Pedido PC | X Remover PC | B Voltar"),19,Lime,1170);
        InfinityButtons::Strip(c,45,608,c.Tr("LB Fila nativa | RB Gerenciar | R3 Instalar XEX/ZIP"),18,Muted,1170);
    }else{
        c.Text(x+20,528,c.Tr("A Detalhes"),21,Lime,w-40);
        InfinityButtons::Strip(c,45,584,c.Tr("Esq/Dir Categorias | Cima/Baixo Itens | A Detalhes"),18,Lime,1170);
        InfinityButtons::Strip(c,45,608,c.Tr("LB Destino HDD/USB | R3 Provedores | Y Atualizar | B Voltar"),18,Muted,1170);
    }
}

namespace {
void approvedIcon(HomeCanvas& c,int kind,int x,int y,int size,unsigned color){
    InfinityIcons::Draw(c,kind,x,y,size,color);
}
}
std::vector<const Game*> ConsoleHome::ApprovedShelf()const{
    std::vector<const Game*> out;
    std::set<std::string> seen;
    auto add=[&](const Game* game){if(game&&!contains(hidden_,game->executable)&&seen.insert(game->executable).second)out.push_back(game);};
    for(const auto& path:history_)add(Installed(path));
    for(const auto& path:pinned_)add(Installed(path));
    for(const auto& path:favorites_)add(Installed(path));
    for(const auto& game:games_)add(&game);
    return out;
}
bool ConsoleHome::ApprovedCarouselTick(uint64_t now){
    // Never rotate the action under the user's focus or during a modal/transfer.
    if(!ApprovedHeader()||!IdleEligible()||focus_==ApprovedHome::Hero||ApprovedHome::CoverSlot(focus_)){approvedHeroAt_=now;return false;}
    if(!approvedHeroAt_||now<approvedHeroAt_){approvedHeroAt_=now;return false;}
    if(now-approvedHeroAt_<8000)return false;
    approvedHeroAt_=now;
    const auto count=ApprovedShelf().size();
    if(count<2)return false;
    approvedHero_=ApprovedHome::Featured(approvedHero_+1,count);return true;
}
bool ConsoleHome::PressApprovedHome(Key key){
    using namespace ApprovedHome;
    const auto shelf=ApprovedShelf();
    approvedPage_=ApprovedHome::PageIndex(approvedPage_,shelf.size());approvedHero_=Featured(approvedHero_,shelf.size());
    const size_t visible=Visible(approvedPage_,shelf.size());
    if(key!=Back)approvedReversePath_.clear();
    if(CoverSlot(focus_)&&size_t(focus_-FirstCover)>=visible)focus_=LibraryLink;
    if(key==Refresh){
        Reload();RefreshStorage();RefreshNetwork();status_=ConsoleStatus::Read();approvedPage_=0;approvedHero_=0;
        if(clockSettings_.city)weatherPending_=true;
        if(notice_.empty())notice_="Biblioteca e indicadores atualizados.";
        if(CoverSlot(focus_)&&size_t(focus_-FirstCover)>=Visible(0,ApprovedShelf().size()))focus_=LibraryLink;
        return true;
    }
    if(focus_==LibraryLink&&key==Guide){
#if defined(INFINITY_XSDK_HTTPS)
        if(TransferRunning()||nativeStorePending_||nativeBatch_){notice_="Conclua a transferencia ativa.";return true;}
        if(!coverBatchConfirm_){PrepareCoverBatch();coverBatchConfirm_=!coverBatch_.empty();notice_=coverBatchConfirm_?"START confirma baixar "+number(coverBatch_.size())+" capas faltantes.":"Nenhuma capa faltante neste lote.";}
        else{coverBatchConfirm_=false;coverBatchPending_=true;}
#else
        notice_="XboxUnity: download nativo indisponivel neste ambiente.";
#endif
        return true;
    }
    if(focus_==Hero&&key==Favorite){approvedHero_=Featured(approvedHero_+1,shelf.size());approvedHeroAt_=TransferRate::Now();return true;}
    if(CoverSlot(focus_)){
        const size_t index=approvedPage_*ShelfSize+size_t(focus_-FirstCover);
        if(key==Back&&index<shelf.size()){
            const auto path=shelf[index]->executable;
            if(approvedReversePath_==path){approvedReversePath_.clear();notice_.clear();}
            else if(appearance_.ShowCover()){approvedReversePath_=path;notice_.clear();}
            else notice_="Exibicao de capas desativada.";
            return true;
        }
        if(key==PageUp||key==PageDown){
            const size_t page=key==PageUp?(approvedPage_?approvedPage_-1:Pages(shelf.size())-1):(approvedPage_+1)%Pages(shelf.size());
            approvedPage_=page;focus_=FirstCover+int(std::min(size_t(focus_-FirstCover),Visible(page,shelf.size())-1));notice_.clear();return true;
        }
        if(index<shelf.size()&&(key==Favorite||key==Guide)){
            const auto path=shelf[index]->executable;auto& items=key==Favorite?favorites_:pinned_;
            const auto old=items;const auto it=std::find(items.begin(),items.end(),path);const bool adding=it==items.end();
            if(adding&&key==Guide&&items.size()>=PinLimit){notice_="Limite de jogos fixados.";return true;}
            if(adding)items.push_back(path);else items.erase(it);
            if(!SaveProfile()){items=old;notice_="Falha ao salvar; alteracao cancelada.";return true;}
            const auto next=ApprovedShelf();const auto pos=std::find_if(next.begin(),next.end(),[&](const Game* item){return item->executable==path;});
            if(pos!=next.end()){size_t n=size_t(pos-next.begin());approvedPage_=n/ShelfSize;focus_=FirstCover+int(n%ShelfSize);}
            notice_=key==Favorite?(adding?"Favorito salvo.":"Favorito removido."):(adding?"Jogo fixado.":"Jogo desafixado.");
            return true;
        }
    }
    if(key<=Down){focus_=Move(focus_,key,static_cast<int>(visible));notice_.clear();return true;}
    return false;
}
void ConsoleHome::OpenApprovedSlot(){
    using namespace ApprovedHome;
    const int selected=focus_;const auto shelf=ApprovedShelf();
    if(selected==Home)return;
    if(selected==LibraryLink){searchTerm_.clear();Enter(Library);return;}
    if(selected==Weather){weatherPanel_=true;notice_.clear();if(clockSettings_.city&&!weather_.valid)weatherPending_=true;return;}
    if(selected==Hero||CoverSlot(selected)){
        const size_t i=selected==Hero?Featured(approvedHero_,shelf.size()):ApprovedHome::PageIndex(approvedPage_,shelf.size())*ShelfSize+static_cast<size_t>(selected-FirstCover);
        if(i>=shelf.size()){Enter(Library);return;}
        const std::string path=shelf[i]->executable;searchTerm_.clear();Enter(Library);
        for(size_t j=0;j<visible_.size();++j)if(visible_[j]->executable==path){row_=static_cast<int>(j);page_=Details;RefreshTitleMetadata();return;}
        return;
    }
    if(selected==ApprovedHome::System||GroupSlot(selected)){
        approvedGroup_=selected==ApprovedHome::System?3:GroupOrder(approvedShortcuts_)[GroupPosition(selected)];approvedGroupRow_=0;notice_.clear();return;
    }
    OpenApprovedAction(selected);
}
void ConsoleHome::OpenApprovedAction(int action){
    using namespace ApprovedHome;
    if(action==ApprovedHome::Games){searchTerm_.clear();Enter(Library);return;}
    if(action==ApprovedHome::Apps){Enter(ConsoleHome::Apps);return;}
    if(action==ApprovedHome::Iptv||action==Tv){Enter(ConsoleHome::Iptv);return;}
    if(action==ApprovedHome::Files){Enter(ConsoleHome::Files);return;}
    if(action==Store||action==Shop){storeActive_=true;storeDetails_=false;storeConfirm_=false;storeRow_=0;storeTab_=0;storeSource_="*";store_.Load();RebuildStore();return;}
    if(action==ApprovedHome::System){Enter(ConsoleHome::System);return;}
    Enter(ConsoleHome::System);
    systemTab_=action==Downloads?3:action==Network||action==Link?1:action==Disc?6:7;
    // Reuse the established system-panel entry checks and initialization.
    Press(Accept);
    if(action==Link){networkPeer_=true;}
    if(action==Achievements||action==Awards){localMilestones_=true;achievementsPage_=true;RefreshAchievements();}
    if(action==Settings){localMilestones_=true;appearanceDraft_=appearance_;clockDraft_=clockSettings_;themeDraft_=theme_;layoutDraft_=layout_;gridDraft_=gridPreference_;layoutPreview_=false;appearanceRow_=0;appearanceEditor_=true;wallpaperDirty_=true;}
    if(action==Power){powerPage_=true;powerConfirm_=ConsolePower::Confirmation();}
}
void ConsoleHome::DrawApprovedGroup(HomeCanvas& c)const{
    using namespace ApprovedHome;
    const char* titles[]={"Gerenciar jogos","Rede e mídia","Perfil e serviços","Sistema"};
    c.DimBackdrop();c.SeriesTile(350,152,580,445,false,250);
    c.TextFit(378,172,titles[approvedGroup_],32,White,524);
    c.TextFit(378,215,"Escolha uma função",18,Muted,524);
    const auto actions=GroupActions(approvedGroup_,approvedShortcuts_);
    const char* labels[]={"Início","Biblioteca","Aplicativos","IPTV","Loja Infinity","Downloads","Central do sistema","Rede","Perfis","Conquistas","Configurações","Energia","Destaque","Instalador DVD/GOD","Gerenciador de arquivos","System Link","Cloud360 IPTV","Loja Infinity","Conquistas"};
    for(int i=0;i<4&&actions[i]>=0;++i){
        const int y=255+i*63;const int item=actions[i];
        c.SeriesTile(375,y,530,55,i==approvedGroupRow_);
        const int icons[]={0,1,2,3,4,5,10,7,8,9,10,11,1,13,32,15,3,4,9};
        InfinityIcons::Draw(c,icons[item],391,y+10,34,i==approvedGroupRow_?Lime:White);c.TextFit(442,y+12,labels[item],23,White,443);
    }
    InfinityButtons::Hint(c,InfinityButtons::A,378,551,28,"Abrir",130,White);
    InfinityButtons::Hint(c,InfinityButtons::B,593,551,28,"Voltar",130,White);
}
void ConsoleHome::DrawApprovedHeader(HomeCanvas& c,bool weatherFocused)const{
    const unsigned accent=0xffa8ff00u;
    c.Translucent(0,0,1280,78,0xff101211u,100);
    c.BrandLogo(28,15);
    c.Box(357,18,1,43,0xff65736du);
    c.SeriesTile(371,12,194,52,false);
    const auto stamp=TransferRate::Now();
    const bool current=headerThermalSampled_&&stamp>=headerThermalAt_&&stamp-headerThermalAt_<=30000;
    const ThermalTelemetry empty;
    const auto& reading=current?headerThermal_:empty;
    const bool warning=ThermalAdvisory::Evaluate(reading,thermalLimit_)==ThermalAdvisory::Reached;
    approvedIcon(c,26,378,27,18,White);
    c.TextFit(401,14,"CPU "+reading.CompactCelsius(0)+" | GPU "+reading.CompactCelsius(1),12,warning?0xffffb34du:White,157);
    c.TextFit(401,31,"EDRAM "+reading.CompactCelsius(2)+" | SYS "+reading.CompactCelsius(3),11,Muted,157);
    c.TextFit(401,47,"IP "+(network_.ipv4.empty()?std::string("-"):network_.ipv4),11,Muted,157);
    DrawSocialHeader(c,572,12,189);
    c.SeriesTile(772,12,266,52,weatherFocused);
    const bool has=weather_.valid&&weather_.city==clockSettings_.city&&clockSettings_.city;
    const bool fresh=has&&weather_.Fresh(std::time(nullptr));
    if(has){
        const unsigned wc=fresh?accent:Muted;
        const int weatherIcon=weather_.code<=1?(weather_.day?InfinityIcons::Sun:InfinityIcons::Moon):weather_.code>=51?InfinityIcons::Rain:25;
        InfinityIcons::Draw(c,weatherIcon,783,22,31,wc);
        c.TextFit(821,15,weather_.Temperature()+" | "+ClockWeather::Cities[weather_.city].name,17,White,209);
        c.Text(821,39,fresh?weather_.Condition():"Clima desatualizado",13,Muted,209);
    }else{approvedIcon(c,25,785,24,26,White);c.TextFit(821,16,clockSettings_.city?ClockWeather::Cities[clockSettings_.city].name:"Clima desativado",17,White,209);c.TextFit(821,39,clockSettings_.city?"Y Consultar clima":"Configure a cidade",14,Muted,209);}
}
void ConsoleHome::DrawApprovedHome(HomeCanvas& c,bool preview)const{
    using namespace ApprovedHome;
    const auto& appearance=preview?appearanceDraft_:appearance_;
    const auto shelf=ApprovedShelf();const int active=preview?Home:focus_;
    const size_t page=ApprovedHome::PageIndex(preview?0:approvedPage_,shelf.size()), visible=Visible(page,shelf.size());
    const Game* featured=shelf.empty()?nullptr:shelf[Featured(preview?0:approvedHero_,shelf.size())];
    const unsigned accent=0xffa8ff00u;
    const char* names[]={"Início","Jogos","Apps","IPTV","Loja","Downloads","Sistema","Rede","Perfis","Conquistas","Configurar","Energia"};
    c.Translucent(0,82,1280,66,0xff161917u,100);
    for(int i=0;i<Hero;++i){
        if(!NavSlot(i))continue;
        const auto a=Geometry(i);if(i==active)c.SeriesTile(a.x,a.y,a.w,a.h,true,210);
        approvedIcon(c,i,a.x+15,a.y+10,26,i==active?accent:White);
        c.TextFit(a.x+50,a.y+10,c.Tr(names[i]),21,White,a.w-62);
    }
    c.Box(28,148,1224,1,0xff414743u);
    const auto hero=Geometry(ApprovedHome::Hero);c.SeriesTile(hero.x,hero.y,hero.w,hero.h,active==Hero);
    bool art=false,illustration=false;
    const auto builtinArt=[&](const Game& game,int x,int y,int w,int h){
        // The original bundled atlas already contains these three illustrations.
        int sx=600,sy=66;
        if(game.name=="Halo 3")sx=742;
        else if(game.name=="Gears of War 3")sy=132;
        else if(game.name!="Forza Horizon")return false;
        c.Image(sx,sy,140,64,x,y,w,h);return true;
    };
    if(!shelf.empty()&&appearance.ShowBackdrop()){
        auto bg=ArtPath(*featured,false);if(!bg.empty())art=c.LocalArt(bg,false,hero.x+2,hero.y+2,hero.w-4,hero.h-4,true,10);
    }
    if(!art&&!shelf.empty()&&appearance.ShowBackdrop()){
        illustration=builtinArt(*featured,hero.x+hero.w/3,hero.y+5,hero.w*2/3-5,hero.h-10);art=illustration;
    }
    if(art)c.Shade(hero.x+8,hero.y+8,hero.w-16,hero.h-16,0xff070c0au,230,4);
    else if(!shelf.empty()&&appearance.ShowCover()){
        auto cover=ArtPath(*featured,true);if(!cover.empty())c.LocalArt(cover,true,hero.x+hero.w-155,hero.y+10,145,210);
    }
    c.Text(hero.x+30,hero.y+22,"XBOX 360  |  "+std::string(c.Tr("Destaque")),17,White,hero.w-60);
    c.TextFit(hero.x+30,hero.y+61,shelf.empty()?std::string("INFINITY 360"):featured->name,45,White,435);
    c.TextFit(hero.x+30,hero.y+124,shelf.empty()?"Adicione jogos ao HDD ou USB":"Biblioteca local | Xbox 360",19,Muted,390);
    c.Box(hero.x+30,hero.y+177,162,36,accent,9);
    InfinityButtons::Draw(c,InfinityButtons::A,hero.x+40,hero.y+183,25);
    c.TextFit(hero.x+74,hero.y+179,shelf.empty()?"Biblioteca":"Detalhes",19,0xff101710u,110);
    if(illustration)c.TextFit(hero.x+hero.w-130,hero.y+hero.h-25,"Arte ilustrativa",12,White,121);
    for(size_t i=0;i<std::min(HeroSize,shelf.size());++i)c.Box(hero.x+hero.w/2-25+int(i)*15,hero.y+hero.h-13,8,8,i==Featured(preview?0:approvedHero_,shelf.size())?accent:Muted,4);
    if(active==Hero)c.SeriesFocus(hero.x,hero.y,hero.w,hero.h);
    const char* quick[]={"Gerenciar","Rede e mídia","Perfil e serviços"};
    const char* lines[][2]={{"DVD / HDD / USB","Downloads"},{"System Link","IPTV / Rede"},{"Loja / Perfil","Conquistas"}};
    const int icons[]={14,15,8};
    const auto order=GroupOrder(approvedShortcuts_);
    for(int i=Disc;i<FirstCover;++i){
        if(!GroupSlot(i))continue;
        const auto a=Geometry(i);c.SeriesTile(a.x,a.y,a.w,a.h,i==active);
        const int item=order[GroupPosition(i)];
        approvedIcon(c,icons[item],a.x+(a.w-49)/2,a.y+27,49,i==active?accent:White);
        c.CenterText(a.x+7,a.y+96,a.w-14,quick[item],19,White);
        c.CenterText(a.x+7,a.y+138,a.w-14,lines[item][0],15,Muted);
        c.CenterText(a.x+7,a.y+163,a.w-14,lines[item][1],15,Muted);
        c.CenterText(a.x+7,a.y+199,a.w-14,"Abrir >",14,accent);
    }
    c.Box(30,410,3,22,accent,1);c.Text(44,403,"Meus Jogos ("+number(shelf.size())+")",25,White,850);
    c.TextFit(935,410,number(page+1)+"/"+number(Pages(shelf.size())),16,Muted,118);
    if(active==LibraryLink)c.SeriesTile(1070,400,182,37,true);
    c.TextFit(1087,410,"Ver biblioteca >",17,White,161);
    for(size_t i=0;i<visible;++i){
        const auto a=Geometry(FirstCover+static_cast<int>(i));const auto& game=*shelf[page*ShelfSize+i];
        c.SeriesTile(a.x,a.y,a.w,a.h,active==FirstCover+static_cast<int>(i));
        const bool reverse=!preview&&active==FirstCover+int(i)&&approvedReversePath_==game.executable;
        bool cover=false;if(appearance.ShowCover()){const auto path=reverse?ReverseArtPath(game):ArtPath(game,true);if(!path.empty()){
            const int w=reverse?(a.h-8)*2/3:a.w-4,h=reverse?a.h-8:a.h-4;
            cover=c.LocalArt(path,true,a.x+(a.w-w)/2,a.y+(a.h-h)/2,w,h,!reverse,8);
        }}
        bool demo=false;
        if(!reverse&&!cover&&appearance.ShowBackdrop()){const auto file=ArtPath(game,false);if(!file.empty())cover=c.LocalArt(file,false,a.x+2,a.y+2,a.w-4,a.h-4,true,10);}
        if(!reverse&&!cover&&appearance.ShowBackdrop()){demo=builtinArt(game,a.x+5,a.y+5,a.w-10,a.h-10);cover=demo;}
        if(cover){for(int dy=0;dy<55;++dy)c.Translucent(a.x+5,a.y+a.h-60+dy,a.w-10,1,0xff090f0bu,unsigned(80+dy*3));}
        else {approvedIcon(c,1,a.x+(a.w-42)/2,a.y+47,42,Muted);c.CenterText(a.x+3,a.y+10,a.w-6,"XBOX 360",13,Muted);c.CenterText(a.x+3,a.y+100,a.w-6,reverse?"Sem verso":"Sem capa",13,Muted);}
        c.TextFit(a.x+12,a.y+a.h-32,game.name,17,White,a.w-24);
        if(reverse)c.TextFit(a.x+9,a.y+8,"VERSO",12,White,a.w-18);
        if(demo)c.TextFit(a.x+9,a.y+8,"Arte ilustrativa",12,White,a.w-18);
        if(Starred(game))InfinityIcons::Draw(c,21,a.x+a.w-25,a.y+7,18,accent);
        if(contains(pinned_,game.executable))c.TextFit(a.x+9,a.y+26,"FIXADO",12,accent,100);
        if(active==FirstCover+static_cast<int>(i))c.SeriesFocus(a.x,a.y,a.w,a.h);
    }
    if(shelf.empty())c.Text(45,477,"Nenhum jogo localizado. Y Atualizar biblioteca.",23,Muted,1160);
    if(preview)c.Text(40,636,"Previa de layout | B Voltar ao editor",17,accent,1180);
    else {
        namespace IB=InfinityButtons;
        if(CoverSlot(active)){IB::Draw(c,IB::LB,30,630,27);IB::Hint(c,IB::RB,74,630,27,"Página",105,White);IB::Hint(c,IB::Start,240,630,27,"Fixar",100,White);}
        else if(active==Hero){IB::Hint(c,IB::X,30,632,22,"Próximo destaque",220,White);IB::Hint(c,IB::R3,300,632,22,"Guia",100,White);}
        else if(active==LibraryLink){IB::Hint(c,IB::A,30,632,22,"Biblioteca",145,White);IB::Hint(c,IB::Start,220,632,22,"Baixar capas faltantes",260,White);}
        else {IB::Hint(c,IB::R3,30,630,27,"Guia",110,White);IB::Hint(c,IB::Start,180,630,27,"Personalizar",200,White); }
    }
}

bool ConsoleHome::WeatherAutoDue(int64_t now){
    if(!clockSettings_.city||!ApprovedHeader()||weatherPanel_||quickGuideOpen_||!IdleEligible()||now<946684800LL)return false;
    // Selecting a city opts into periodic updates. No polling on other pages,
    // during transfer or before the clock is valid; failure is also throttled.
    if(lastWeatherAttempt_&&now>=lastWeatherAttempt_&&now-lastWeatherAttempt_<1800)return false;
    if(lastWeatherAttempt_&&now<lastWeatherAttempt_)lastWeatherAttempt_=0;
    lastWeatherAttempt_=now;weatherPending_=true;return true;
}
void ConsoleHome::DrawWeatherPanel(HomeCanvas& c)const{
    c.Text(45,88,"Clima | Previsao de cinco dias",38,White,1180);
    if(!clockSettings_.city){c.Text(48,200,"Selecione uma cidade em Configurar > Aparencia.",24,Muted,1160);return;}
    c.Text(48,150,ClockWeather::Cities[clockSettings_.city].name,27,Lime,1170);
    const auto now=static_cast<int64_t>(std::time(nullptr));
    if(!weather_.valid||weather_.city!=clockSettings_.city){c.Text(48,235,"Sem leitura meteorologica. Y Consultar.",23,Muted,1160);return;}
    c.Glass(45,192,1190,112,false);
    c.Text(65,207,weather_.Temperature(),40,White,300);
    c.Text(350,208,weather_.Condition(),25,Lime,860);
    c.Text(350,249,weather_.Fresh(now)?"Condicoes atuais | Open-Meteo":"Dados desatualizados | Y Consultar",19,Muted,860);
    if(!weather_.dayCount)c.Text(48,352,"Previsao diaria indisponivel.",23,Muted,1160);
    for(unsigned i=0;i<weather_.dayCount;++i){
        const auto& day=weather_.days[i];int x=45+240*i;
        c.Glass(x,327,230,238,false);
        // Forecast dates use the selected city's offset, not the display clock.
        c.CenterText(x,341,230,ClockWeather::Format(day.epoch,weather_.cityOffsetSeconds/60,"%d/%m","--/--"),24,White);
        ClockWeather::Reading condition;condition.code=day.code;condition.day=true;
        c.CenterText(x,395,230,condition.Condition(),19,Lime);
        char temps[40];std::snprintf(temps,sizeof temps,"%.0f / %.0f C",day.minimum,day.maximum);
        c.CenterText(x,441,230,temps,26,White);
        c.CenterText(x,493,230,day.rain<0?"Chuva -":"Chuva "+number(day.rain)+"%",18,Muted);
    }
    InfinityButtons::Strip(c,48,600,"Y Atualizar | B Voltar | Minima / maxima | Fonte: Open-Meteo",18,Lime,1170);
}
