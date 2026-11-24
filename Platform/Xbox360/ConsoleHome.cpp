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

namespace {
const size_t HistoryLimit=24;
const char* const labels[]={"Biblioteca","Cloud360 IPTV","Aplicativos","Emuladores","Arquivos","Sistema","Recentes","Favoritos"};
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
      fileWorkspace_(dataDir),fileRoot_(0),pasteConfirm_(false),
      focus_(0),row_(0),subtab_(0),systemTab_(0),gameSort_(0),searchCursor_(0),
      collectionIndex_(0),collectionRow_(0),keyboardMode_(0),
      searchActive_(false),collectionPicker_(false),deleteCollectionConfirm_(false),
      contentActive_(false),contentKnown_(false),contentTab_(0),contentRow_(0),
      contentInfo_(),section_(Library),page_(Home) {}

void ConsoleHome::Load() {
    roots_=GameScanner::LoadRoots(dir_+"aurorax-scanpaths.txt",baseRoots_);
    collections_.Load();
    const std::string path=dir_+"aurorax-profile.txt";
    const std::string src=savedFile(path);
    std::ifstream in(src.c_str());
    history_.clear(); favorites_.clear(); hidden_.clear(); launched_.clear();
    std::string line;
    while (std::getline(in,line)) {
        if (!line.empty() && line[line.size()-1]=='\r') line.erase(line.size()-1);
        if (line.size()<3 || line.size()>2048 || line[1]!='\t') continue;
        const std::string file=line.substr(2);
        if(line[0]=='R' && history_.size()<HistoryLimit && !contains(history_,file)) history_.push_back(file);
        if(line[0]=='F' && !contains(favorites_,file)) favorites_.push_back(file);
        if(line[0]=='H' && !contains(hidden_,file)) hidden_.push_back(file);
        if(line[0]=='P' && launched_.size()<4096 && !contains(launched_,file))
            launched_.push_back(file);
    }
    ScanLibrary();
    Rebuild();
}
void ConsoleHome::ScanLibrary() {
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
        Game item={pkg.name,pkg.path,kind,false};
        games_.push_back(item);
    }
}
bool ConsoleHome::SaveProfile() const {
    const std::string path=dir_+"aurorax-profile.txt", tmp=path+".tmp";
    std::ofstream out(tmp.c_str(),std::ios::trunc);
    for (size_t i=0;i<history_.size();++i) out<<"R\t"<<history_[i]<<"\n";
    for (size_t i=0;i<favorites_.size();++i) out<<"F\t"<<favorites_[i]<<"\n";
    for (size_t i=0;i<hidden_.size();++i) out<<"H\t"<<hidden_[i]<<"\n";
    for (size_t i=0;i<launched_.size();++i) out<<"P\t"<<launched_[i]<<"\n";
    out.close();
    if (!out) return false;
    return replaceSavedFile(path);
}
bool ConsoleHome::Starred(const Game& item) const { return contains(favorites_,item.executable); }
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
        const std::vector<std::string>& paths = section_==Recent ? history_ : favorites_;
        for(size_t i=0;i<paths.size();++i) { const Game* g=Installed(paths[i]); if(g) visible_.push_back(g); }
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
                 (!hidden && (subtab_==6?neverOpened:
                              subtab_==5?inCollection:
                              libraryTabMatch(games_[i],subtab_)));
            if(allowed && matchesLibraryQuery(games_[i],searchTerm_))
                visible_.push_back(&games_[i]);
        }
        std::stable_sort(visible_.begin(),visible_.end(),LibraryOrder(gameSort_));
    } else if (section_==Apps || section_==Emulators) {
        // Tabs are filters over the same installed XEX catalogue.
        // 0 Todos; 1 Instalados; 2 Plugins; 3 Homebrew (Apps only).
        for(size_t i=0;i<tools_.size() && i<toolKinds_.size();++i)
            if(subtab_==0 || toolKinds_[i]==subtab_) visible_.push_back(&tools_[i]);
    }
    clampRow(row_,Count());
}
void ConsoleHome::Reload() {
    notice_.clear();
    roots_=GameScanner::LoadRoots(dir_+"aurorax-scanpaths.txt",baseRoots_);
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
bool ConsoleHome::Tick() {
    if(!TransferRunning())return false;
    const unsigned previous=transfer_.Percent();
    transfer_.Tick();
    if(!TransferRunning()) {
        if(section_==Files && page_==Listing && !searchActive_)Reload();
        notice_=transfer_.Status()==FileTransfer::Completed?"Arquivo copiado.":
            "Copia interrompida: "+transfer_.Error();
        return true;
    }
    return previous!=transfer_.Percent();
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
}
void ConsoleHome::RefreshNetwork() {
    network_=NetworkInfo::Read();
}
void ConsoleHome::Enter(Section s) {
    section_=s; page_=Listing; row_=0; subtab_=0; systemTab_=0;
    collectionPicker_=false; searchActive_=false; keyboardMode_=0;
    if(s==Iptv) {tv_.Open();return;}
    if(s==System) RefreshStorage();
    if (s==Files) {folders_.clear();fileRoot_=0;pasteConfirm_=false;folders_.push_back(fileWorkspace_.Roots()[0]);}
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
void ConsoleHome::OpenContent() {
    contentActive_=true;
    contentKnown_=false;
    contentInfo_=XboxExecutionInfo();
    contentItems_.clear();
    contentTab_=0;
    contentRow_=0;
    notice_.clear();
    if(!Count() || section_==Files || section_==System)return;
    const Game& game=*visible_[row_];
    if(isContentPackage(game)) {
        XboxContentPackage package;
        if(ContentCatalog::ReadHeader(game.executable,package)) {
            contentInfo_.titleId=package.titleId;
            contentInfo_.mediaId=package.mediaId;
            contentInfo_.version=package.version;
            contentKnown_=true;
        }
    } else contentKnown_=ContentCatalog::ReadExecutionInfo(game.executable,contentInfo_);
    if(contentKnown_)
        contentItems_=ContentCatalog(ContentCatalog::DefaultRoots(dir_)).ScanExtras(contentInfo_.titleId);
}
size_t ConsoleHome::ContentCount() const {
    const unsigned types[]={0x000b0000u,2u,1u};
    size_t count=0;
    for(size_t i=0;i<contentItems_.size();++i)
        if(contentItems_[i].contentType==types[contentTab_])++count;
    return count;
}
const XboxContentPackage* ConsoleHome::ContentAt(size_t index) const {
    const unsigned types[]={0x000b0000u,2u,1u};
    for(size_t i=0;i<contentItems_.size();++i) {
        if(contentItems_[i].contentType!=types[contentTab_])continue;
        if(index==0)return &contentItems_[i];
        --index;
    }
    return 0;
}
void ConsoleHome::LaunchFailed() { notice_="O console não iniciou o título. Confira se o dispositivo está conectado."; }

std::string ConsoleHome::Press(Key key) {
    if(contentActive_) {
        if(key==Back) {contentActive_=false;notice_.clear();return "";}
        if(key==Refresh) {const int tab=contentTab_;OpenContent();contentTab_=tab;return "";}
        if(key==Left && contentTab_>0) {--contentTab_;contentRow_=0;}
        if(key==Right && contentTab_<2) {++contentTab_;contentRow_=0;}
        if(key==Up && contentRow_>0)--contentRow_;
        if(key==Down && static_cast<size_t>(contentRow_+1)<ContentCount())++contentRow_;
        if(key==Accept) {
            const XboxContentPackage* item=ContentAt(contentRow_);
            if(item)notice_=item->path;
        }
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
        if(pasteConfirm_) {
            pasteConfirm_=false;
            if(key==Accept) {
                if(TransferRunning())notice_="Ja existe uma transferencia ativa.";
                else if(transfer_.Start(fileWorkspace_,copyDir_,copyName_,folders_.back()))
                    notice_="Copia iniciada. Veja Sistema > Downloads.";
                else notice_=transfer_.Error();
            } else notice_="Copia cancelada.";
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
            if(Count() && !files_[row_].directory) {
                copyDir_=folders_.back();copyName_=files_[row_].name;
                notice_="Arquivo marcado para copiar: "+copyName_;
            } else notice_="Selecione um arquivo (nao uma pasta).";
            return "";
        }
        if(key==PageUp) {
            if(TransferRunning()) {notice_="Ja existe uma transferencia ativa.";return "";}
            if(copyName_.empty() || !fileWorkspace_.CanWrite(folders_.back()))
                notice_="Marque um arquivo com X; abra UserFiles/AuroraXFiles para colar.";
            else {pasteConfirm_=true;notice_="A confirma a copia sem sobrescrever; B cancela.";}
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
            else if(systemTab_==3)notice_="Transferencias locais atualizadas automaticamente.";
            else notice_="Este servico ainda nao possui provider nativo.";
            return "";
        }
        if(page_==Details) page_=Listing;
        Reload();
        if(notice_.empty()) notice_="Atualizado";
        return "";
    }
    if(section_==System && page_!=Home) {
        if(page_==Details && systemTab_==3 && key==Favorite && TransferRunning()) {
            transfer_.Cancel();notice_="Transferencia cancelada; origem preservada.";return "";
        }
        // Real console panels: do not invent temperature/network/FTP readings.
        if(page_==Listing) {
            if(key==Left && systemTab_>0) --systemTab_;
            if(key==Right && systemTab_<3) ++systemTab_;
            if(key==Accept) {
                if(systemTab_==0)RefreshStorage();
                else if(systemTab_==1)RefreshNetwork();
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
        if(key==Accept) Enter(static_cast<Section>(focus_));
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
        const int last=section_==Emulators?2:section_==Library?7:3;
        if(key==Left && subtab_>0) --subtab_;
        if(key==Right && subtab_<last) ++subtab_;
        row_=0;
        notice_.clear();
        Rebuild();
        return "";
    }
    if(page_==Details) {
        if(key==Accept && section_!=Files && !visible_.empty()) {
            const std::string path=visible_[row_]->executable;
            if(!isXexPath(path) || isContentPackage(*visible_[row_])) {
                notice_="Pacote de conteudo: launch nativo ainda nao integrado.";
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

void ConsoleHome::DrawHome(HomeCanvas& c) const {
    c.Text(44,76,"Início",42,White,330);
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
        c.Image(600+i*52,0,52,52,x+24,376,39,38);
        c.Text(x+80,375,labels[Apps+i],23,White,205);
    }
    c.Box(767,456,1,174,0xff355148u);
    for(int side=0;side<2;++side) {
        int x=side?789:42, w=side?449:701;
        bool focused=focus_==(side?Favorites:Recent);
        if(focused) c.Box(x,490,w,3,Lime);
        c.Text(x,448,labels[side?Favorites:Recent],28,focused?Lime:White,w);
        const std::vector<std::string>& paths=side?favorites_:history_;
        int shown=0, cardWidth=(w-18)/3;
        for(size_t i=0;i<paths.size() && shown<3;++i) {
            const Game* g=Installed(paths[i]); if(!g) continue;
            int left=x+shown*(cardWidth+9);
            c.Box(left,504,cardWidth,126,Panel,8);
            int sx=600,sy=66;
            if(g->name=="Halo 3") sx=742;
            else if(g->name=="Gears of War 3") sy=132;
            if(g->name=="Forza Horizon" || g->name=="Halo 3" || g->name=="Gears of War 3")
                c.Image(sx,sy,140,64,left+1,505,cardWidth-2,84);
            else {
                c.Box(left+1,505,cardWidth-2,84,0xff194739u,6);
                c.Image(652,0,52,52,left+cardWidth/2-20,525,40,40,Muted);
            }
            c.Text(left+8,594,g->name,16,White,cardWidth-16);
            ++shown;
        }
        if(!shown) {
            c.Box(x,503,w,124,Panel,9);
            c.Text(x+15,543,side?"Use X em um jogo para favoritar.":"Os jogos abertos aparecem aqui.",19,Muted,w-26);
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
    const char* const tabs[]={"Title Updates","DLC","Saves"};
    for(int i=0;i<3;++i) {
        const int x=45+i*310;
        panel(c,x,183,295,47,Panel,i==contentTab_);
        c.Text(x+16,191,tabs[i],23,i==contentTab_?Lime:White,265);
    }
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

void ConsoleHome::DrawStorage(HomeCanvas& c) const {
    c.Text(45,349,"Dispositivos e espaco disponivel",27,White,1170);
    for(size_t i=0;i<storageVolumes_.size() && i<4;++i) {
        const StorageVolume& v=storageVolumes_[i];
        const int x=42+(static_cast<int>(i)%2)*602;
        const int y=391+(static_cast<int>(i)/2)*106;
        c.Box(x,y,589,97,Panel,10);
        const std::string label=i==0?"Pasta AuroraX":v.path;
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
    c.Text(45,610,"Y Atualizar | B Voltar | Caminhos podem compartilhar o mesmo disco.",18,Muted,1180);
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
    c.Text(660,447,"FTP: nucleo pronto; servidor inativo",21,Muted,558);
    c.Text(660,486,"SMB / System Link: pendentes",20,Muted,558);
    c.Text(660,534,"Internet e Xbox Live: nao testados",19,Muted,558);
    c.Text(45,607,"Y Atualizar | B Voltar | IP local nao comprova acesso a Internet.",18,Muted,1180);
}
void ConsoleHome::DrawTransfers(HomeCanvas& c) const {
    c.Text(45,349,"Transferencias locais",27,White,1170);
    c.Box(42,391,1196,202,Panel,10);
    const FileTransfer::State state=transfer_.Status();
    if(state==FileTransfer::Idle) {
        c.Text(60,422,"Nenhuma transferencia nesta sessao.",24,White,1150);
        c.Text(60,476,"Arquivos: X Copiar, LB Colar e A Confirmar.",20,Muted,1150);
    } else {
        const char* const states[]={"Inativo","Copiando","Concluido","Cancelado","Falhou"};
        c.Text(60,406,transfer_.Name(),24,White,1145);
        c.Text(60,450,states[state]+std::string(" | ")+
               StorageInfo::FormatBytes(transfer_.Done())+" / "+
               StorageInfo::FormatBytes(transfer_.Total()),20,Muted,1145);
        c.Box(60,493,1155,16,0xff28413a,5);
        const unsigned percent=transfer_.Percent();
        if(percent)c.Box(60,493,static_cast<int>(1155*percent/100),16,Lime,5);
        c.Text(60,526,number(percent)+"%"+(transfer_.Error().empty()?
               std::string(""):" | "+transfer_.Error()),20,White,1145);
    }
    c.Text(45,609,TransferRunning()?"X Cancelar copia | B Voltar (copia continua)":
           "B Voltar | Downloads de internet ainda pendentes",18,Muted,1180);
}
void ConsoleHome::DrawSection(HomeCanvas& c) const {
    if(contentActive_) {DrawContent(c);return;}
    c.Text(44,78,labels[section_],43,White,1190);
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
            "Armazenamento","Rede e FTP","Temperaturas","Downloads"
        };
        static const char* const marks[]={"HDD","NET","SMC","DL"};
        static const char* const summaries[]={
            "Jogos e discos","Servicos de rede","Telemetria SMC","Tarefas locais"
        };
        static const char* const details[]={
            "Capacidade e espaco livre dos dispositivos.",
            "FTP: autenticacao local implementada; transporte de rede pendente.",
            "Temperaturas e ventoinha: leitura SMC ainda nao implementada.",
            "Transferencias locais; downloads de internet pendentes."
        };
        for(int i=0;i<4;++i) {
            const int x=42+i*302;
            panel(c,x,166,290,166,Panel,i==systemTab_);
            c.Text(x+17,184,marks[i],20,i==systemTab_?Lime:Muted,96);
            c.Text(x+17,222,titles[i],28,White,264);
            c.Text(x+17,284,summaries[i],18,Muted,252);
        }
        if(page_==Details && systemTab_==0) {DrawStorage(c);return;}
        if(page_==Details && systemTab_==1) {DrawNetwork(c);return;}
        if(page_==Details && systemTab_==3) {DrawTransfers(c);return;}
        if(page_==Details) {
            c.Box(42,369,1196,220,Panel,12);
            c.Text(68,391,titles[systemTab_],34,White,1120);
            c.Text(69,470,details[systemTab_],23,Muted,1122);
            c.Text(69,543,"B para voltar aos paineis.",20,Lime,1120);
        } else {
            c.Text(47,369,"Resumo AuroraX",29,White,1100);
            c.Box(42,422,1196,165,Panel,12);
            c.Text(67,443,"Jogos encontrados    "+number(games_.size()),25,White,1120);
            c.Text(67,494,"Catalogo IPTV local   "+number(tv_.Count())+" itens",23,Muted,1120);
            c.Text(67,543,"Interface nativa      1280 x 720",22,Muted,1120);
        }
        return;
    }
    if(page_==Details && Count()) {
        c.Box(44,169,1190,329,Panel,12);
        std::string title,sub,hint;
        if(section_==Files) {
            title=files_[row_].name; sub=folders_.back(); hint="Visualização de arquivos";
        } else {
            title=visible_[row_]->name; sub=visible_[row_]->executable;
            hint=(isXexPath(sub) && !isContentPackage(*visible_[row_]))?
                "Pressione A para iniciar este título.":
                "Pacote identificado. Execucao nativa ainda nao homologada.";
        }
        c.Text(76,219,title,36,White,1110);
        c.Text(77,298,sub,22,Muted,1100);
        c.Text(77,384,hint,21,Lime,1110);
        if(section_==Library || section_==Recent || section_==Favorites)
            c.Text(77,430,"RB Conteudo instalado (TU / DLC / Saves)",20,Muted,1110);
        return;
    }
    const bool tabbed=(section_==Apps || section_==Emulators || section_==Library);
    if(tabbed) {
        static const char* const appTabs[]={"Todos","Instalados","Plugins","Homebrew"};
        static const char* const gameTabs[]={"Todos","Xbox 360","XBLA","Original","Pacotes","Colecoes","Nunca","Ocultos"};
        const char* const* tabs=section_==Library?gameTabs:appTabs;
        const int numTabs=section_==Library?8:section_==Emulators?3:4;
        const int tabWidth=numTabs==8?109:numTabs==3?244:207;
        for(int i=0;i<numTabs;++i) {
            const int x=45+i*(tabWidth+9);
            panel(c,x,148,tabWidth,43,Panel,i==subtab_);
            c.Text(x+15,155,tabs[i],21,i==subtab_?Lime:White,tabWidth-25);
        }
        c.Text(49,200,number(Count())+" itens nesta aba",21,Muted,1120);
        if(section_==Library) {
            static const char* const sorts[]={"Nome A-Z","Nome Z-A","Dispositivo"};
            c.Text(861,201,std::string("RS Ordenar: ")+sorts[gameSort_],18,Muted,355);
            if(subtab_==5 && !collections_.Items().empty()) {
                const size_t current=static_cast<size_t>(collectionIndex_);
                if(current<collections_.Items().size())
                    c.Text(315,200,collections_.Items()[current].name,21,Lime,510);
            }
            c.Text(45,621,subtab_==5?
                   "Start Buscar | LB Proxima colecao | RB Gerenciar | RS Ordenar":
                   "Start Buscar | LB Ocultar | RB Colecoes | RS Ordenar",
                   18,Muted,1170);
        }
    } else if(section_==Files) {
        c.Text(49,146,folders_.back(),21,White,1140);
        c.Text(49,176,fileWorkspace_.CanWrite(folders_.back())?
            "Espaco pessoal | escrita habilitada":"Navegacao somente leitura | Start cria espaco pessoal no dispositivo",
            18,Muted,1140);
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
        } else {name=visible_[i]->name; detail=(Starred(*visible_[i])?"*  ":"")+visible_[i]->device;}
        c.Text(62,y+6,name,23,White,810);
        c.Text(886,y+9,detail,19,Muted,336);
    }
}
void ConsoleHome::Draw(HomeCanvas& c,const std::string& clock,int64_t now) const {
    c.Background();
    c.Image(814,0,64,64,44,17,46,46); c.Text(103,16,"AURORA",32,White,145);
    c.Text(240,16,"X",32,Lime,44);
    c.Text(1088,19,clock,26,White,90);
    c.Box(1189,15,49,49,Lime,24); c.Box(1192,18,43,43,Panel,22);
    c.Text(1199,25,"AX",19,White,32);
    if(page_!=Home && section_==Iptv) {
        tv_.Draw(c,now<0?static_cast<int64_t>(std::time(0)):now);
        return;
    }
    if(page_==Home) DrawHome(c); else DrawSection(c);
    if(TransferRunning() && !(section_==System && page_==Details && systemTab_==3)) {
        c.Text(720,71,"Copia "+number(transfer_.Percent())+"% | Sistema > Downloads",16,Lime,510);
    }
    if(!notice_.empty()) {
        c.Box(40,628,1200,35,0xff102923u,6);
        c.Text(51,630,notice_,18,Lime,1170);
    }
    c.Box(0,663,1280,57,0xff061915u);
    buttonHint(c,45,"A","Selecionar",0xff428914u);
    buttonHint(c,254,"B","Voltar",0xffb83c28u);
    buttonHint(c,429,"X",section_==System && page_==Details && systemTab_==3?"Cancelar":
        section_==Files && page_!=Home && !searchActive_?"Copiar":"Favorito",0xff117db1u);
    buttonHint(c,632,"Y","Atualizar",0xffb3820du);
}
