#include "ConsoleHome.h"
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

ConsoleHome::ConsoleHome(const std::string& dataDir,const std::vector<std::string>& roots)
    : dir_(dataDir),roots_(roots),baseRoots_(roots),tv_(dataDir),
      collections_(dataDir+"aurorax-collections.txt"),
      fileWorkspace_(dataDir),ftp_(fileWorkspace_),copyPaused_(false),storeDownloads_(false),storeDownloadRow_(0),storagePage_(0),fileRoot_(0),pasteConfirm_(false),scanPathConfirm_(false),fileAction_(false),deleteConfirm_(false),cutMode_(false),fileActionRow_(0),multiSelect_(false),
      focus_(0),row_(0),subtab_(0),systemTab_(0),gameSort_(0),searchCursor_(0),
      homeOrder_(DefaultHomeOrder),homeDraft_(DefaultHomeOrder),homeEditSlot_(0),homeEditing_(false),storeActive_(false),storeDetails_(false),storeConfirm_(false),storeTab_(0),storeRow_(0),store_(dataDir),storeTranslationMatch_(false),nativeStorePending_(false),importPreview_(false),
      collectionIndex_(0),collectionRow_(0),keyboardMode_(0),
      searchActive_(false),collectionPicker_(false),deleteCollectionConfirm_(false),
      contentActive_(false),contentKnown_(false),contentTab_(0),contentRow_(0),
      contentInfo_(),artDirty_(true),artEditing_(false),artEditRow_(0),trainerConfirm_(false),section_(Library),page_(Home) {}

void ConsoleHome::Load() {
    LoadEvents();
    LoadScanPaths();
    if(!collections_.Load())notice_="Colecoes invalidas: edicao bloqueada para preservar o arquivo.";
    const std::string path=dir_+"aurorax-profile.txt";
    const std::string src=savedFile(path);
    std::ifstream in(src.c_str());
    history_.clear(); favorites_.clear(); pinned_.clear(); hidden_.clear(); launched_.clear();
    homeOrder_=DefaultHomeOrder;homeEditing_=false;
    artChoices_.clear();artEditing_=false;importPreview_=false;
    std::string line;
    while (std::getline(in,line)) {
        if (!line.empty() && line[line.size()-1]=='\r') line.erase(line.size()-1);
        if (line.size()<3 || line.size()>2048 || line[1]!='\t') continue;
        const std::string file=line.substr(2);
        if(line[0]=='O' && validHomeOrder(file)) homeOrder_=file;
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
        if(line[0]=='P' && launched_.size()<4096 && !contains(launched_,file))
            launched_.push_back(file);
    }
    // Legacy profiles recorded only R; these were also launch requests.
    for(size_t i=0;i<history_.size() && launched_.size()<4096;++i)
        if(!contains(launched_,history_[i])) launched_.push_back(history_[i]);
    ScanLibrary();
    Rebuild();
}
void ConsoleHome::ScanLibrary() {
    artFolders_.clear();artDirty_=true;
    games_=GameScanner(roots_).Scan();
    const std::vector<XboxContentPackage> packages=
        ContentCatalog(ContentCatalog::DefaultRoots(dir_)).Scan();
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
        tools_=GameScanner(roots).Scan();
        toolKinds_.assign(tools_.size(),1);
        // Installed homebrew plugins are opt-in local XEX applications.
        const std::vector<PluginDescriptor> plugins=
            PluginRegistry(PluginRegistry::DefaultRoots(dir_)).Scan();
        const std::string category=section_==Apps?"app":"emulator";
        for(size_t i=0;i<plugins.size();++i) if(plugins[i].category==category) {
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
    activeCopyTarget_=FileWorkspace::Join(task.destination,task.name);
    notice_="Proxima copia iniciada.";return true;
}
void ConsoleHome::LoadEvents() {
    events_.clear();
    std::ifstream in(savedFile(dir_+"infinity360-events.log").c_str(),std::ios::binary);
    if(!in)return;
    std::vector<char> raw(32769);
    in.read(&raw[0],static_cast<std::streamsize>(raw.size()));
    if(in.bad()||in.gcount()>32768)return;
    std::istringstream lines(std::string(&raw[0],static_cast<size_t>(in.gcount())));
    std::string line;
    while(std::getline(lines,line)) {
        if(line.empty()||line.size()>240)continue;
        bool clean=true;
        for(size_t i=0;i<line.size();++i)if((unsigned char)line[i]<32||line[i]==127)clean=false;
        if(!clean)continue;
        events_.push_back(line);
        if(events_.size()>64)events_.erase(events_.begin());
    }
}
void ConsoleHome::RecordEvent(const std::string& message) {
    // Fixed operational events only: no filenames, URLs, account details or tokens.
    const std::time_t now=std::time(0);
    std::tm* utc=std::gmtime(&now);
    char stamp[32]="hora indisponivel";
    if(utc)std::strftime(stamp,sizeof stamp,"%Y-%m-%d %H:%MZ",utc);
    events_.push_back(std::string(stamp)+" | "+message);
    if(events_.size()>64)events_.erase(events_.begin());
    const std::string file=dir_+"infinity360-events.log";
    std::ofstream out((file+".tmp").c_str(),std::ios::binary|std::ios::trunc);
    for(size_t i=0;i<events_.size();++i)out<<events_[i]<<"\n";
    out.close();
    if(!out||!replaceSavedFile(file))notice_+=" (historico apenas nesta sessao)";
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
    const bool ftpChanged=ftp_.Tick();
    if(!TransferRunning() || copyPaused_)return ftpChanged;
    if(transfer_.Status()!=FileTransfer::Running) {
        StartNextCopy();
        if(!TransferRunning() && section_==Files && page_==Listing && !searchActive_)Reload();
        return true;
    }
    const unsigned previous=transfer_.Percent();
    transfer_.Tick();
    if(transfer_.Status()!=FileTransfer::Running) {
        activeCopyTarget_.clear();
        if(transfer_.Status()!=FileTransfer::Completed)copyQueue_.clear();
        if(section_==Files && page_==Listing && !searchActive_)Reload();
        notice_=transfer_.Status()==FileTransfer::Completed?"Arquivo copiado.":
            "Copia interrompida: "+transfer_.Error();
        RecordEvent(transfer_.Status()==FileTransfer::Completed?"Arquivo copiado":"Copia interrompida");return true;
    }
    return ftpChanged || previous!=transfer_.Percent();
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
    residentConfig_.clear();residentPlugins_.clear();
    // Show only the first readable candidate, not a claim about active boot source.
    std::vector<std::string> paths;
    paths.push_back(dir_+"launch.ini");
#ifdef _XBOX
    paths.push_back("HDD1:\\launch.ini");
    paths.push_back("USB0:\\launch.ini");
    paths.push_back("USB1:\\launch.ini");
#endif
    for(size_t i=0;i<paths.size();++i) {
        if(readResidentPlugins(paths[i],residentPlugins_)) {
            residentConfig_=paths[i];return;
        }
    }
    residentPlugins_.clear();
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
    section_=s; page_=Listing; row_=0; subtab_=0; systemTab_=0;
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
    return artFile(folder,cover,choice);
}
void ConsoleHome::RefreshTitleMetadata() {
    detailMetadata_=XboxTitleMetadata();
    if(!Count() || section_==Files || section_==System)return;
    const Game& game=*visible_[row_];
    detailMetadata_=ContentCatalog::DescribeTitle(game.executable,isContentPackage(game));
}

void ConsoleHome::OpenContent() {
    contentActive_=true;
    contentKnown_=false;
    contentInfo_=XboxExecutionInfo();
    contentItems_.clear();trainers_.clear();trainerConfirm_=false;
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
}

size_t ConsoleHome::ContentCount() const {
    if(contentTab_==3)return trainers_.size();
    const unsigned types[]={0x000b0000u,2u,1u};
    size_t count=0;
    for(size_t i=0;i<contentItems_.size();++i)
        if(contentItems_[i].contentType==types[contentTab_])++count;
    return count;
}
const XboxContentPackage* ConsoleHome::ContentAt(size_t index) const {
    if(contentTab_==3)return 0;
    const unsigned types[]={0x000b0000u,2u,1u};
    for(size_t i=0;i<contentItems_.size();++i) {
        if(contentItems_[i].contentType!=types[contentTab_])continue;
        if(index==0)return &contentItems_[i];
        --index;
    }
    return 0;
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
        if(storeTab_==0||store_.Items()[i].category==tabs[storeTab_])storeVisible_.push_back(i);
    clampRow(storeRow_,storeVisible_.size());
}
bool ConsoleHome::StartNativeStore(const StoreItem& item){
#if defined(INFINITY_XSDK_HTTPS) || defined(_XBOX)
    if(TransferRunning()||!TranslationMatches(item)){notice_="Copia ativa ou traducao incompativel.";return false;}
    if(store_.Staged(item)){notice_="Arquivo local ja existe; nenhum download iniciado.";return false;}
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
    notice_="Fila nativa concluida.";return false;
}
bool ConsoleHome::TakeStoreInstallRequest(StoreItem& item){
    if(!storeInstallPending_)return false;
    storeInstallPending_=false;item=storeInstallItem_;return true;
}
void ConsoleHome::StoreInstallResult(const std::string& message,bool success){
    notice_=message;RecordEvent(success?"XEX autorizado instalado":"Instalacao interrompida");
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
std::string ConsoleHome::Press(Key key) {
    if(storeActive_) {
        if(key!=Search&&notice_=="R3 novamente: confirmar instalacao XEX?")notice_.clear();
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
                    notice_="Instalacao: apenas XEX autorizado, baixado e ainda nao instalado.";return "";
                }
                if(notice_!="R3 novamente: confirmar instalacao XEX?"){notice_="R3 novamente: confirmar instalacao XEX?";return "";}
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
        if(key==Back) {artEditing_=false;notice_="Alteracoes de arte canceladas.";return "";}
        if(key==Up || key==Down)artEditRow_=1-artEditRow_;
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
        if(key==Right && contentTab_<3) {++contentTab_;contentRow_=0;}
        if(key==Up && contentRow_>0)--contentRow_;
        if(key==Down && static_cast<size_t>(contentRow_+1)<ContentCount())++contentRow_;
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
    if(page_==Details && (section_==Library || section_==Recent || section_==Favorites)
       && key==Guide && Count()) {
        artEditFolder_=ArtFolder(*visible_[row_]);
        if(artEditFolder_.empty()) {notice_="Arte indisponivel: Title ID desconhecido.";return "";}
        artEditId_=artEditFolder_.substr(artEditFolder_.size()-9,8);
        const std::map<std::string,std::string>::const_iterator it=artChoices_.find(artEditId_);
        artDraft_=it==artChoices_.end()?"00":it->second;
        artEditRow_=0;artEditing_=true;notice_.clear();
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
    if (key==Back) {
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
        if(section_==System && page_!=Home) {
            if(systemTab_==0) {RefreshStorage();notice_="Armazenamento consultado novamente.";}
            else if(systemTab_==1) {RefreshNetwork();notice_="Diagnostico de rede atualizado.";}
            else if(systemTab_==3) {
                if(storeDownloads_) {store_.Load();RebuildStoreDownloads();notice_="Registros de downloads atualizados.";}
                else notice_="Transferencias locais atualizadas automaticamente.";
            }
            else if(systemTab_==4) {RefreshResidentPlugins();notice_="Configuracao de plugins consultada.";}
            else if(systemTab_==5)notice_="Historico local dos ultimos 64 eventos.";
            else notice_="Este servico ainda nao possui provider nativo.";
            return "";
        }
        if(page_==Details) page_=Listing;
        Reload();
        if(notice_.empty()) notice_="Atualizado";
        return "";
    }
    if(section_==System && page_!=Home) {
        if(page_==Details && systemTab_==6){
#if defined(INFINITY_XSDK_HTTPS)
            if(key==Up||key==Down){discDestination_=(discDestination_+(key==Down?1:3))%4;discConfirm_=false;}
            if(key==Accept){
                if(TransferRunning()){notice_="Conclua ou cancele a copia ativa.";return "";}
                if(!discConfirm_){discConfirm_=true;notice_="Confirme apenas DVD de uso autorizado.";}
                else{discConfirm_=false;discPending_=true;notice_="Preparando instalacao DVD.";}
            }
#endif
            return "";
        }
        if(page_==Details && systemTab_==0 && (key==PageUp||key==PageDown)) {
            const size_t pages=(storageVolumes_.size()+3)/4;
            if(pages)storagePage_=(storagePage_+pages+(key==PageDown?1:pages-1))%pages;
            return "";
        }
        if(page_==Details && systemTab_==1 && (key==Accept||key==Favorite)) {ToggleFtp(key==Favorite);return "";}
        if(page_==Details && systemTab_==3 && key==PageDown) {
            storeDownloads_=!storeDownloads_;storeDownloadRow_=0;
            if(storeDownloads_) {store_.Load();RebuildStoreDownloads();}
            return "";
        }
        if(page_==Details && systemTab_==3 && storeDownloads_) {
            const size_t count=storeDownloadItems_.size();
            if(key==Up&&storeDownloadRow_>0)--storeDownloadRow_;
            if(key==Down&&static_cast<size_t>(storeDownloadRow_+1)<count)++storeDownloadRow_;
#if defined(INFINITY_XSDK_HTTPS)
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
            copyPaused_=!copyPaused_;
            notice_=copyPaused_?"Fila pausada. A para continuar.":"Fila retomada.";
            RecordEvent(copyPaused_?"Fila pausada":"Fila retomada");return "";
        }
        // Real console panels: do not invent temperature/network/FTP readings.
        if(page_==Listing) {
            if(key==Left && systemTab_>0) --systemTab_;
            if(key==Right && systemTab_<6) ++systemTab_;
            if(key==Accept) {
                if(systemTab_==0)RefreshStorage();
                else if(systemTab_==1)RefreshNetwork();
                else if(systemTab_==4)RefreshResidentPlugins();
                else if(systemTab_==6){discConfirm_=false;discDestination_=0;}
                else if(systemTab_==3) {storeDownloads_=false;storeDownloadRow_=0;}
                page_=Details;
            }
        }
        return "";
    }
    if(page_==Home) {
        static const int next[][4]={
            {0,1,0,2},{0,1,1,4},{2,3,0,6},{2,4,0,6},
            {3,5,1,7},{4,5,1,7},{6,7,2,6},{6,7,4,7}
        };
        if(key<=Down) { focus_=next[focus_][key]; notice_.clear(); }
        if(key==PageDown) {
            storeActive_=true;storeDetails_=false;storeConfirm_=false;storeRow_=0;storeTab_=0;
            notice_=store_.Load()?"":"Catalogo ausente: configure aurorax-store.tsv.";
            RebuildStore();return "";
        }
        if(key==Favorite) {PreviewAuroraProfile();return "";}
        if(key==Guide) {
            homeDraft_=homeOrder_;
            homeEditSlot_=(focus_>=Apps && focus_<=System)?focus_-Apps:0;
            homeEditing_=true;notice_.clear();
            return "";
        }
        if(key==Accept) {
            const int destination=(focus_>=Apps && focus_<=System)?
                Apps+homeOrder_[focus_-Apps]-'0':focus_;
            Enter(static_cast<Section>(destination));
        }
        return "";
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
            if(!isXexPath(path) || isContentPackage(*visible_[row_])) {
                if(isContentPackage(*visible_[row_])) {
                    const XboxPackageCheck check=ContentCatalog::CheckPackage(path);
                    notice_=ContentCatalog::PackageStatus(check.status);
                } else notice_="Formato de executavel nao suportado.";
                return "";
            }
            if(TransferRunning()) {
                notice_="Conclua ou cancele a copia antes de iniciar outro executavel.";
                return "";
            }
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
    c.Box(x,y,w,h,active?Lime:0xff2e4842u,11);
    int inset=active?4:1;
    c.Box(x+inset,y+inset,w-2*inset,h-2*inset,bg,10-inset/2);
}
void buttonHint(HomeCanvas& c,int x,const char* key,const char* action,unsigned color) {
    c.Box(x,674,29,29,color,14);
    c.Text(x+7,673,key,21,White,26);
    c.Text(x+38,672,action,20,White,150);
}
}

void ConsoleHome::DrawResidentPlugins(HomeCanvas& c) const {
    c.Text(47,349,"Plugins residentes / DashLaunch",27,White,1170);
    c.Text(47,385,residentConfig_.empty()?"Nenhum launch.ini legivel":residentConfig_,18,Muted,1170);
    for(size_t i=0;i<residentPlugins_.size();++i) {
        const ResidentPluginSlot& slot=residentPlugins_[i];
        const int y=415+static_cast<int>(i)*32;
        c.Text(51,y,"Slot "+number(i+1),18,White,110);
        c.Text(171,y,slot.path.empty()?"Vazio":slot.path,18,Muted,840);
        c.Text(1025,y,slot.duplicate?"Ambiguo":isLiveEmulationReference(slot.path)?"LiveEmulation":"",17,Lime,195);
    }
    c.Text(47,588,"Somente leitura: estado carregado e conexao nao verificados.",18,Muted,1170);
    c.Text(47,610,"Y Atualizar | B Voltar",18,Lime,1170);
}
void ConsoleHome::DrawHomeEditor(HomeCanvas& c) const {
    c.Text(44,82,"Personalizar inicio",40,White,1190);
    c.Text(46,150,"Organize os quatro atalhos da segunda fileira.",24,Muted,1180);
    c.Text(46,196,"Biblioteca e Cloud360 IPTV permanecem no topo.",21,Muted,1180);
    for(int i=0;i<4;++i) {
        const int x=42+i*302, item=homeDraft_[i]-'0';
        panel(c,x,282,289,168,Panel,i==homeEditSlot_);
        c.Image(600+item*52,0,52,52,x+112,303,60,60);
        c.Text(x+16,374,labels[Apps+item],26,i==homeEditSlot_?Lime:White,258);
        c.Text(x+16,414,"Posicao "+number(i+1),18,Muted,252);
    }
    c.Text(46,500,"Direcional: selecionar | LB/RB: mover o atalho",23,White,1180);
    c.Text(46,550,"X Restaurar padrao | Y Salvar | B Cancelar",23,Lime,1180);
}
void ConsoleHome::DrawAuroraProfile(HomeCanvas& c) const {
    c.Text(44,78,"Importar perfil Aurora",40,White,1180);
    c.Text(48,152,"Previa da mesclagem com jogos encontrados neste console",23,Muted,1180);
    c.Box(44,215,1190,272,Panel,12);
    c.Text(70,244,"Novos favoritos: "+number(importFavorites_.size()),30,Lime,1130);
    c.Text(70,308,"Registros de historico encontrados: "+number(importHistory_.size()),26,White,1130);
    c.Text(70,370,"Recentes locais tem prioridade. Jogos ambiguos ou ausentes sao ignorados.",21,Muted,1130);
    c.Text(70,417,"Nenhum jogo ou banco Aurora sera alterado.",22,Muted,1130);
    c.Text(48,560,"A Confirmar mesclagem | B Cancelar",24,Lime,1180);
}
void ConsoleHome::DrawStore(HomeCanvas& c) const {
    c.Text(44,78,"INFINITY Store",40,White,1175);
    c.Text(48,133,"Catalogos autorizados | Download nativo experimental",21,Muted,1175);
    const char* const tabs[]={"Todos","Jogos","XBLA","Homebrew","Apps","Emuladores","DLC","PT-BR"};
    for(int i=0;i<8;++i) {
        const int x=45+i*149;
        panel(c,x,177,140,44,Panel,i==storeTab_);
        c.Text(x+8,185,tabs[i],18,i==storeTab_?Lime:White,124);
    }
    if(storeVisible_.empty()) {
        c.Text(48,270,"Nenhum item no catalogo desta categoria.",24,White,1175);
        c.Text(48,320,"Configure aurorax-store.tsv com fontes e licencas autorizadas.",20,Muted,1175);
    } else if(storeDetails_) {
        const StoreItem& item=store_.Items()[storeVisible_[storeRow_]];
        c.Text(55,260,item.name,32,White,1150);
        c.Text(55,315,"Versao: "+item.version+" | "+StorageInfo::FormatBytes(item.bytes),24,Muted,1150);
        c.Text(55,367,"Licenca / autorizacao declarada: "+item.rights,22,Muted,1150);
        c.Text(55,415,store_.Installed(item)?"XEX instalado (presenca local)":store_.Progress(item).state==StoreProgress::NativePaused?"Parcial salvo | START retoma | RB duas vezes descarta":store_.Progress(item).state==StoreProgress::NativeVerified?"Verificado no Xbox (registro local)":store_.Staged(item)?"Arquivo local encontrado (tamanho)":
            store_.NativeQueued(item)?"Na fila nativa":store_.Requested(item)?"Solicitado na fila PC":"Disponivel no catalogo",22,Lime,1150);
        if(item.category=="translation") {
            char ids[128];std::sprintf(ids,"Title %08X | Media %08X | XEX %08X",
                                       item.titleId,item.mediaId,item.gameVersion);
            c.Text(55,467,ids,20,Muted,1150);
            c.Text(55,505,storeTranslationMatch_?"Cabecalho local coincide; TU ativa nao validada":
                "Nenhum XEX local correspondente; pedido bloqueado",20,storeTranslationMatch_?Lime:Muted,1150);
        } else c.Text(55,470,item.url,18,Muted,1150);
#if defined(INFINITY_XSDK_HTTPS)
        c.Text(55,560,store_.Staged(item)&&!store_.Installed(item)&&(item.category=="app"||item.category=="homebrew"||item.category=="emulator")?"R3 duas vezes: instalar XEX independente":store_.NativeQueued(item)?"LB Remover da fila nativa":"LB Adicionar a fila nativa",20,Lime,1150);
#endif
        c.Text(55,590,storeConfirm_?"A Confirmar pedido | B Cancelar":"Start Baixar no Xbox | A Pedido PC | X Remover PC | B Voltar",21,Lime,1150);
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
    c.Text(48,590,"A Detalhes | Y Atualizar catalogo | B Voltar",21,Lime,1175);
}
void ConsoleHome::DrawHome(HomeCanvas& c) const {
    if(storeActive_) {DrawStore(c);return;}
    if(importPreview_) {DrawAuroraProfile(c);return;}
    if(homeEditing_) {DrawHomeEditor(c);return;}
    c.Text(44,76,"Início",42,White,330);
    c.Text(459,86,"RB Loja | X Importar | Start Personalizar",20,Muted,775);
    panel(c,42,140,672,196,0xff042417u,focus_==Library);
    panel(c,728,140,510,196,0xff003e33u,focus_==Iptv);
    c.Image(0,0,342,222,436,150,260,173);
    c.Image(344,0,254,222,1035,155,183,163);
    c.Image(652,0,52,52,70,160,47,43,Lime);
    c.Text(69,214,"Biblioteca",43,White,350);
    c.Text(70,278,"Jogos instalados",23,Muted,350);
    c.Text(752,219,"Cloud360 IPTV",35,White,283);
    c.Text(753,275,"TV ao vivo, filmes e séries",20,Muted,280);
    for(int i=0;i<4;++i) {
        int x=42+i*302;
        panel(c,x,357,289,75,Panel,focus_==Apps+i);
        const int item=homeOrder_[i]-'0';
        c.Image(600+item*52,0,52,52,x+24,376,39,38);
        c.Text(x+80,375,labels[Apps+item],23,White,205);
    }
    c.Box(767,456,1,174,0xff355148u);
    for(int side=0;side<2;++side) {
        int x=side?789:42, w=side?449:701;
        bool focused=focus_==(side?Favorites:Recent);
        if(focused) c.Box(x,490,w,3,Lime);
        c.Text(x,448,labels[side?Favorites:Recent],side?24:28,focused?Lime:White,w);
        const std::vector<std::string> paths=side?HomeShortcuts():history_;
        int shown=0, cardWidth=(w-18)/3;
        for(size_t i=0;i<paths.size() && shown<3;++i) {
            const Game* g=Installed(paths[i]); if(!g || contains(hidden_,paths[i])) continue;
            int left=x+shown*(cardWidth+9);
            c.Box(left,504,cardWidth,126,Panel,8);
            const std::string backFile=ArtPath(*g,false), coverFile=ArtPath(*g,true);
            c.Box(left+1,505,cardWidth-2,84,0xff194739u,6);
            const bool backdrop=!backFile.empty() && c.LocalArt(backFile,false,left+1,505,cardWidth-2,84);
            const bool cover=!coverFile.empty() && c.LocalArt(coverFile,true,left+(cardWidth-56)/2,505,56,84);
            if(!backdrop && !cover) {
                int sx=600,sy=66;
                if(g->name=="Halo 3") sx=742;
                else if(g->name=="Gears of War 3") sy=132;
                if(g->name=="Forza Horizon" || g->name=="Halo 3" || g->name=="Gears of War 3")
                    c.Image(sx,sy,140,64,left+1,505,cardWidth-2,84);
                else c.Image(652,0,52,52,left+cardWidth/2-20,525,40,40,Muted);
            }
            c.Text(left+8,594,g->name,16,White,cardWidth-16);
            if(side && contains(pinned_,paths[i])) c.Text(left+8,508,"FIXADO",14,Lime,cardWidth-16);
            ++shown;
        }
        if(!shown) {
            c.Box(x,503,w,124,Panel,9);
            c.Text(x+15,543,side?"LB nos detalhes fixa um jogo.":"Os jogos abertos aparecem aqui.",19,Muted,w-26);
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
        c.Text(62,y+4,item.name,23,White,715);
        c.Text(62,y+31,"Media "+hexId(item.mediaId)+" | Versao "+hexId(item.version),17,Muted,715);
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
void ConsoleHome::DrawContent(HomeCanvas& c) const {
    c.Text(44,78,"Conteudo instalado",40,White,1170);
    if(!contentKnown_) {
        c.Text(48,150,"Title ID indisponivel para este executavel.",26,White,1170);
        c.Text(48,198,"Nao e possivel associar TU, DLC ou saves com seguranca.",21,Muted,1170);
        c.Text(48,260,"B Voltar | Y Atualizar",22,Lime,1170);
        return;
    }
    c.Text(45,135,"Title ID: "+hexId(contentInfo_.titleId)+"    Media ID: "+
           hexId(contentInfo_.mediaId)+"    Versao: "+hexId(contentInfo_.version),
           21,Muted,1180);
    const char* const tabs[]={"Title Updates","DLC","Saves","Trainers"};
    for(int i=0;i<4;++i) {
        const int x=45+i*299;
        panel(c,x,183,287,47,Panel,i==contentTab_);
        c.Text(x+16,191,tabs[i],23,i==contentTab_?Lime:White,257);
    }
    if(contentTab_==3) {DrawTrainers(c);return;}
    c.Text(48,241,number(ContentCount())+" pacotes encontrados - somente leitura",20,Muted,1175);
    if(ContentCount()==0)c.Text(48,305,"Nenhum pacote encontrado neste local.",24,White,1170);
    const int first=(contentRow_/5)*5;
    for(int i=first;i<first+5;++i) {
        const XboxContentPackage* item=ContentAt(i);
        if(!item)break;
        const int y=280+(i-first)*62;
        panel(c,45,y,1189,55,Panel,i==contentRow_);
        c.Text(62,y+6,item->name,23,White,700);
        std::string status;
        if(contentTab_==0) {
            status=(!contentInfo_.mediaId || !item->mediaId)?"Media ID desconhecido":
                item->mediaId==contentInfo_.mediaId?"Media ID igual":"Media ID diferente";
            c.Text(62,y+31,"Versao: "+hexId(item->version),17,Muted,690);
        } else if(contentTab_==1)status="Marketplace / DLC";
        else status="Save localizado";
        c.Text(780,y+14,status,20,Muted,430);
    }
    c.Text(48,601,"A Caminho | Y Atualizar | B Voltar | Nenhuma ativacao automatica",18,Lime,1175);
}

bool ConsoleHome::TakeDiscInstallRequest(std::string& mount){
    if(!discPending_)return false;
    discPending_=false;
    static const char* drives[]={"hdd:/","usb0:/","usb1:/","usb2:/"};
    mount=drives[discDestination_];return true;
}
void ConsoleHome::DiscInstallResult(const std::string& message,bool success){
    if(success)ScanLibrary();
    notice_=message;RecordEvent(success?"DVD copiado e verificado":"Instalacao DVD interrompida");
}
void ConsoleHome::DrawDiscInstall(HomeCanvas& c) const{
    c.Text(45,349,"Instalar a partir do DVD",28,White,1170);
#if defined(INFINITY_XSDK_HTTPS)
    XboxExecutionInfo info{};
    if(ContentCatalog::ReadExecutionInfo("dvd:/default.xex",info)){
        char id[80];std::sprintf(id,"Title %08X | Media %08X | Disco %u/%u",info.titleId,info.mediaId,info.discNumber,info.discCount);
        c.Text(48,390,id,22,Lime,1170);
    }else c.Text(48,390,"Insira DVD com default.xex acessivel.",22,Muted,1170);
    static const char* drives[]={"hdd:/","usb0:/","usb1:/","usb2:/"};
    c.Text(48,443,std::string("Destino: < ")+drives[discDestination_]+" > /Infinity360Files/Games",23,White,1170);
    c.Text(48,488,"Copia os arquivos acessiveis e verifica cada arquivo.",20,Muted,1170);
    c.Text(48,532,discConfirm_?"A Confirmar instalacao | B Cancelar":"Cima/Baixo Destino | A Preparar | B Voltar",21,Lime,1170);
    c.Text(48,570,"Sem GOD ou alteracao de licenca. Requer marcador DVD e escrita.",18,Muted,1170);
#else
    c.Text(48,400,"Instalador DVD disponivel no XSDK.",22,Muted,1170);
#endif
}
void ConsoleHome::DrawStorage(HomeCanvas& c) const {
    c.Text(45,349,"Dispositivos e espaco disponivel",27,White,1170);
    for(size_t i=0;i<4 && storagePage_*4+i<storageVolumes_.size();++i) {
        const size_t index=storagePage_*4+i;
        const StorageVolume& v=storageVolumes_[index];
        const int x=42+(static_cast<int>(i)%2)*602;
        const int y=391+(static_cast<int>(i)/2)*106;
        c.Box(x,y,589,97,Panel,10);
        const std::string label=index==0?"Pasta INFINITY 360":v.path;
        c.Text(x+14,y+7,label,22,White,550);
        if(!v.available) {
            c.Text(x+14,y+44,"Indisponivel / capacidade nao informada",18,Muted,552);
            continue;
        }
        const unsigned percent=v.UsedPercent();
        c.Text(x+14,y+34,StorageInfo::FormatBytes(v.freeBytes)+" livres / "+
               StorageInfo::FormatBytes(v.capacity)+" total",18,Muted,552);
        c.Box(x+14,y+63,551,10,0xff28413a,5);
        if(percent)c.Box(x+14,y+63,static_cast<int>(551*percent/100),10,Lime,5);
        c.Text(x+14,y+76,number(percent)+"% ocupado",15,White,550);
    }
    c.Text(45,610,"LB/RB Pagina "+number(storagePage_+1)+"/"+number((storageVolumes_.size()+3)/4)+" | Y Atualizar | B Voltar | Volumes podem compartilhar disco.",18,Muted,1180);
}
void ConsoleHome::DrawNetwork(HomeCanvas& c) const {
    c.Text(45,349,"Diagnostico de rede local",27,White,1170);
    c.Box(42,391,589,190,Panel,10);
    c.Text(58,404,"Conexao e endereco IPv4",25,White,550);
    c.Text(58,447,NetworkInfo::StateLabel(network_.state),20,Muted,550);
    c.Text(58,486,"IP: "+(network_.ipv4.empty()?std::string("Nao identificado"):network_.ipv4),
           23,network_.ipv4.empty()?Muted:Lime,550);
    c.Text(58,534,network_.ethernetKnown?
           (network_.ethernetActive?"Ethernet: link ativo":"Ethernet: sem link"):
           "Ethernet: leitura indisponivel",19,Muted,550);
    c.Box(644,391,594,190,Panel,10);
    c.Text(660,404,"Servicos de rede",25,White,558);
    c.Text(660,447,ftp_.Running()?(ftp_.Connected()?"FTP: cliente conectado":"FTP: aguardando cliente"):"FTP: desligado",21,ftp_.Running()?Lime:Muted,558);
    c.Text(660,486,ftp_.Running()?"Porta "+number(ftp_.Port())+" | "+(ftp_.UploadEnabled()?"Leitura/upload":"Somente leitura"):"A Leitura | X Leitura/upload (ALLO)",20,Muted,558);
    c.Text(660,534,"Internet e Xbox Live: nao testados",19,Muted,558);
    c.Text(45,607,"A Leitura | X Leitura/upload | A ou X Parar | Y Atualizar | B Voltar",18,Muted,1180);
}
void ConsoleHome::DrawEvents(HomeCanvas& c) const {
    c.Text(45,349,"Historico operacional (ultimos 64)",27,White,1170);
    if(events_.empty())c.Text(48,410,"Nenhum evento registrado.",23,Muted,1160);
    const size_t shown=events_.size()<6?events_.size():6;
    for(size_t i=0;i<shown;++i)
        c.Text(48,390+static_cast<int>(i)*32,events_[events_.size()-1-i],19,White,1160);
    c.Text(48,610,"6 mais recentes na tela | Arquivo infinity360-events.log | B Voltar",18,Muted,1180);
}
void ConsoleHome::RebuildStoreDownloads() {
    storeDownloadItems_=store_.NativeQueue();
    for(size_t i=0;i<store_.Items().size();++i)
        if(store_.ListedDownload(store_.Items()[i])&&std::find(storeDownloadItems_.begin(),storeDownloadItems_.end(),i)==storeDownloadItems_.end())storeDownloadItems_.push_back(i);
    clampRow(storeDownloadRow_,storeDownloadItems_.size());
}
void ConsoleHome::DrawStoreDownloads(HomeCanvas& c) const {
    c.Text(45,349,"INFINITY Store | Downloads",25,White,1170);
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
    c.Text(48,590,"A Baixar/Retomar | START Rodar fila | X Remover da fila",18,Muted,1170);
    c.Text(48,612,"RB Copias locais | Y Atualizar | B Voltar",18,Lime,1170);
#else
    c.Text(48,590,"Registros locais do Xbox e PC; nao indicam conexao ativa.",18,Muted,1170);
    c.Text(48,612,"RB Copias locais | Y Atualizar | B Voltar",18,Lime,1170);
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
        c.Text(60,526,number(percent)+"%"+(transfer_.Error().empty()?
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
    c.Text(46,137,"Title ID: "+artEditId_,21,Muted,1180);
    static const char* const modes[]={"Principal","Alternativa","Visual padrao"};
    for(int i=0;i<2;++i) {
        const int x=i?430:45, w=i?805:370;
        panel(c,x,180,w,375,Panel,i==artEditRow_);
        c.Text(x+16,190,i?"Fundo":"Capa",27,White,w-32);
        const std::string file=artFile(artEditFolder_,i==0,artDraft_[i]);
        const bool drawn=!file.empty() && (i?
            c.LocalArt(file,false,x+190,244,400,225):
            c.LocalArt(file,true,x+105,244,150,225));
        if(!drawn)c.Text(x+20,330,artDraft_[i]=='2'?"Visual original":"Arquivo ausente ou invalido",21,Muted,w-40);
        c.Text(x+16,488,std::string("< ")+modes[artDraft_[i]-'0']+" >",24,Lime,w-32);
        c.Text(x+16,523,artDraft_[i]=='2'?"Sem arte personalizada":i?
            (artDraft_[i]=='1'?"background-alt.rgba":"background.rgba"):
            (artDraft_[i]=='1'?"cover-alt.rgba":"cover.rgba"),17,Muted,w-32);
    }
    c.Text(46,570,"Cima/Baixo Selecionar | Esquerda/Direita ou A Alterar | RB Recarregar",19,White,1180);
    c.Text(46,604,"X Restaurar principal | Y Salvar | B Cancelar",19,Lime,1180);
}
void ConsoleHome::DrawFileActions(HomeCanvas& c) const {
    c.Text(45,145,"Acoes do arquivo",31,White,1180);
    c.Text(48,196,actionName_,24,Lime,1160);
    const char* const actions[]={"Recortar arquivo","Excluir item",
        multiSelect_ && copyDir_==actionDir_?"Limpar multisselecao":"Iniciar multisselecao","Voltar"};
    for(int i=0;i<4;++i) {
        const int y=240+i*72;
        panel(c,45,y,1189,60,Panel,i==fileActionRow_);
        c.Text(63,y+12,actions[i],25,White,1100);
    }
    c.Text(48,550,"Somente espaco pessoal. Exclusao nao recursiva; movimento no mesmo volume.",20,Muted,1175);
    c.Text(48,600,deleteConfirm_?"A Confirmar exclusao definitiva | B Cancelar":"A Selecionar | B Voltar",20,Lime,1175);
}
void ConsoleHome::DrawSection(HomeCanvas& c) const {
    if(artEditing_) {DrawArtEditor(c);return;}
    if(contentActive_) {DrawContent(c);return;}
    c.Text(44,78,labels[section_],43,White,1190);
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
            "Armazenamento","Rede e FTP","Temperaturas","Downloads","Plugins","Historico","DVD"
        };
        static const char* const marks[]={"HDD","NET","SMC","DL","PLG","LOG","DVD"};
        static const char* const summaries[]={
            "Jogos e discos","Servicos de rede","Telemetria SMC","Tarefas locais","Residentes","Eventos locais","Instalar disco"
        };
        static const char* const details[]={
            "Capacidade e espaco livre dos dispositivos.",
            "FTP LAN: leitura e upload opcional; ativacao manual.",
            "Temperaturas e ventoinha: leitura SMC ainda nao implementada.",
            "Transferencias locais; downloads de internet pendentes.",
            "Diagnostico somente leitura do launch.ini.",
            "Historico de operacoes da dashboard.",
            "Instalacao dos arquivos acessiveis pelo leitor DVD."
        };
        for(int i=0;i<7;++i) {
            const int x=42+i*171;
            panel(c,x,166,161,166,Panel,i==systemTab_);
            c.Text(x+10,184,marks[i],20,i==systemTab_?Lime:Muted,96);
            c.Text(x+10,222,titles[i],19,White,172);
            c.Text(x+10,284,summaries[i],16,Muted,172);
        }
        if(page_==Details && systemTab_==0) {DrawStorage(c);return;}
        if(page_==Details && systemTab_==1) {DrawNetwork(c);return;}
        if(page_==Details && systemTab_==3) {DrawTransfers(c);return;}
        if(page_==Details && systemTab_==4) {DrawResidentPlugins(c);return;}
        if(page_==Details && systemTab_==5) {DrawEvents(c);return;}
        if(page_==Details && systemTab_==6) {DrawDiscInstall(c);return;}
        if(page_==Details) {
            c.Box(42,369,1196,220,Panel,12);
            c.Text(68,391,titles[systemTab_],34,White,1120);
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
        c.Box(44,169,1190,407,Panel,12);
        std::string title,sub,hint;
        if(section_==Files) {
            title=files_[row_].name; sub=folders_.back(); hint="Visualização de arquivos";
        } else {
            title=visible_[row_]->name; sub=visible_[row_]->executable;
            hint=(isXexPath(sub) && !isContentPackage(*visible_[row_]))?
                "Pressione A para iniciar este título.":
                "Pacote identificado. Execucao nativa ainda nao homologada.";
        }
        const std::string backFile=section_==Files?"":ArtPath(*visible_[row_],false);
        const std::string coverFile=section_==Files?"":ArtPath(*visible_[row_],true);
        const bool backdrop=!backFile.empty() && c.LocalArt(backFile,false,842,202,360,202);
        const bool cover=!coverFile.empty() && c.LocalArt(coverFile,true,1090,406,100,150);
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
                       StorageInfo::FormatBytes(m.declaredBytes):
                    m.discCount?"Disco "+number(m.discNumber)+" de "+number(m.discCount):
                       "Disco: informacao indisponivel";
                c.Text(77,407,extra,21,Muted,textWidth);
                if(translationReferences_.count(m.titleId))
                    c.Text(77,439,"PT-BR: titulo referenciado; versao/download nao confirmados",18,Muted,textWidth);
            } else c.Text(77,351,"Metadados indisponiveis para este item.",22,Muted,1100);
        }
        c.Text(77,469,hint,21,Lime,textWidth);
        if(section_==Library || section_==Recent || section_==Favorites)
            c.Text(47,593,"Start Personalizar capa e fundo | Y Atualizar",18,Muted,1170);
        if(section_==Library || section_==Recent || section_==Favorites) {
            c.Text(77,512,"RB Conteudo instalado (TU / DLC / Saves / Trainers)",20,Muted,1110);
            c.Text(77,545,contains(pinned_,visible_[row_]->executable)?"LB Desafixar do inicio":"LB Fixar no inicio",20,Lime,1110);
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
            c.Text(x+(numTabs==9?7:15),155,tabs[i],numTabs==9?17:21,i==subtab_?Lime:White,tabWidth-(numTabs==9?14:25));
        }
        c.Text(49,200,number(Count())+" itens nesta aba",21,Muted,1120);
        if(section_==Library) {
            static const char* const sorts[]={"Nome A-Z","Nome Z-A","Dispositivo"};
            c.Text(861,201,subtab_==8?"Ordem manual":std::string("RS Ordenar: ")+sorts[gameSort_],18,Muted,355);
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
        c.Text(49,615,"RB Dispositivo | Start Nova pasta | RS Renomear | X Copiar | LB Colar",18,Lime,1180);
    }
    else c.Text(49,157,number(Count())+" títulos",21,Muted,1140);
    if(!Count() && notice_.empty())
        c.Text(56,tabbed?275:246,
               section_==Library && subtab_==5?
               "Colecao vazia. RB gerencia colecoes ou cria uma nova.":
               "Nenhum item nesta seção.",25,Muted,1120);
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
        c.Text(nameX,y+6,name,23,White,872-nameX);
        c.Text(886,y+9,detail,19,Muted,336);
    }
}
void ConsoleHome::Draw(HomeCanvas& c,const std::string& clock,int64_t now) const {
    if(artDirty_) {c.ClearLocalArt();artDirty_=false;}
    c.Background();
    // Infinity mark: two overlapping outlined loops, no legacy atlas logo.
    c.Box(43,26,31,25,Lime,12);c.Box(47,30,23,17,0xff102923u,8);
    c.Box(65,26,31,25,Lime,12);c.Box(69,30,23,17,0xff102923u,8);
    c.Text(105,16,"INFINITY",32,White,220);
    c.Text(282,16,"360",32,Lime,84);
    c.Text(1088,19,clock,26,White,90);
    c.Box(1189,15,49,49,Lime,24); c.Box(1192,18,43,43,Panel,22);
    c.Text(1197,26,"360",16,White,37);
    if(page_!=Home && section_==Iptv) {
        tv_.Draw(c,now<0?static_cast<int64_t>(std::time(0)):now);
        return;
    }
    if(page_==Home) DrawHome(c); else DrawSection(c);
    if(TransferRunning() && !(section_==System && page_==Details && systemTab_==3)) {
        c.Text(690,71,(copyPaused_?std::string("Fila pausada"):std::string("Copia ")+number(transfer_.Percent())+"%")+
               " | "+number(copyQueue_.size())+" aguardando",16,Lime,540);
    }
    if(!notice_.empty()) {
        c.Box(40,628,1200,35,0xff102923u,6);
        c.Text(51,630,notice_,18,Lime,1170);
    }
    c.Box(0,663,1280,57,0xff061915u);
    buttonHint(c,45,"A",importPreview_?"Importar":section_==System && page_==Details && systemTab_==3 && TransferRunning()?(copyPaused_?"Retomar":"Pausar"):"Selecionar",0xff428914u);
    buttonHint(c,254,"B","Voltar",0xffb83c28u);
    buttonHint(c,429,"X",page_==Home && !homeEditing_ && !importPreview_?"Importar":(artEditing_ || (page_==Home && homeEditing_))?"Restaurar":section_==System && page_==Details && systemTab_==3?"Cancelar":
        section_==Files && page_!=Home && !searchActive_?"Copiar":"Favorito",0xff117db1u);
    buttonHint(c,632,"Y",(artEditing_ || (page_==Home && homeEditing_))?"Salvar":"Atualizar",0xffb3820du);
}
