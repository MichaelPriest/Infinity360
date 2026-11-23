#include "ConsoleHome.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace {
const size_t HistoryLimit=24;
const char* const labels[]={"Biblioteca","Cloud360 IPTV","Aplicativos","Emuladores","Arquivos","Sistema","Recentes","Favoritos"};
const char* const channelTabs[]={"Todos","TV ao vivo","Filmes","Séries","Favoritos"};
std::string number(size_t value) { std::ostringstream out; out<<value; return out.str(); }
bool contains(const std::vector<std::string>& list,const std::string& path) {
    return std::find(list.begin(),list.end(),path)!=list.end();
}
bool exists(const std::string& path) {
    std::ifstream in(path.c_str(),std::ios::binary);
    return in.good();
}
// FATX cannot rename over an existing file. The backup also covers a power loss
// between renames. Readers use it if the primary file is missing.
bool replaceSavedFile(const std::string& path) {
    const std::string tmp=path+".tmp", backup=path+".bak";
    const bool old=exists(path);
    if(old) {
        std::remove(backup.c_str());
        if(std::rename(path.c_str(),backup.c_str())) return false;
    }
    if(std::rename(tmp.c_str(),path.c_str())) {
        if(old) std::rename(backup.c_str(),path.c_str());
        return false;
    }
    std::remove(backup.c_str());
    return true;
}
std::string savedFile(const std::string& path) { return exists(path) ? path : path+".bak"; }
void clampRow(int& row,size_t count) {
    row = count ? std::min(row,static_cast<int>(count-1)) : 0;
}
}

ConsoleHome::ConsoleHome(const std::string& dataDir,const std::vector<std::string>& roots)
    : dir_(dataDir),roots_(roots),channels_(CLOUD_MAX),filtered_(CLOUD_MAX),
      focus_(0),row_(0),tab_(0),section_(Library),page_(Home) {
    channelFavorites_.count=0;
    cloud_init(&catalog_,&channels_[0],channels_.size());
}

void ConsoleHome::Load() {
    const std::string path=dir_+"aurorax-profile.txt";
    const std::string src=savedFile(path);
    std::ifstream in(src.c_str());
    history_.clear(); favorites_.clear();
    std::string line;
    while (std::getline(in,line)) {
        if (!line.empty() && line[line.size()-1]=='\r') line.erase(line.size()-1);
        if (line.size()<3 || line.size()>2048 || line[1]!='\t') continue;
        const std::string file=line.substr(2);
        if(line[0]=='R' && history_.size()<HistoryLimit && !contains(history_,file)) history_.push_back(file);
        if(line[0]=='F' && !contains(favorites_,file)) favorites_.push_back(file);
    }
    games_=GameScanner(roots_).Scan();
    channelFavorites_.count=0;
    cloud_favorites_load(&channelFavorites_,savedFile(dir_+"favorites.txt").c_str());
    Rebuild();
}
bool ConsoleHome::SaveProfile() const {
    const std::string path=dir_+"aurorax-profile.txt", tmp=path+".tmp";
    std::ofstream out(tmp.c_str(),std::ios::trunc);
    for (size_t i=0;i<history_.size();++i) out<<"R\t"<<history_[i]<<"\n";
    for (size_t i=0;i<favorites_.size();++i) out<<"F\t"<<favorites_[i]<<"\n";
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
    if (section_==Iptv) return filtered_.size();
    if (section_==System) return 0;
    return visible_.size();
}
void ConsoleHome::Rebuild() {
    visible_.clear();
    if (section_==Iptv) {
        filtered_.resize(CLOUD_MAX);
        size_t n=cloud_filter(&catalog_,static_cast<CloudTab>(tab_),&channelFavorites_,0,0,&filtered_[0],filtered_.size());
        filtered_.resize(n);
    } else if (section_==Recent || section_==Favorites) {
        const std::vector<std::string>& paths = section_==Recent ? history_ : favorites_;
        for(size_t i=0;i<paths.size();++i) { const Game* g=Installed(paths[i]); if(g) visible_.push_back(g); }
    } else if (section_==Library || section_==Apps || section_==Emulators) {
        const std::vector<Game>& items=section_==Library ? games_ : tools_;
        for(size_t i=0;i<items.size();++i) visible_.push_back(&items[i]);
    }
    clampRow(row_,Count());
}
void ConsoleHome::Reload() {
    notice_.clear();
    if (section_==Iptv && page_!=Home) {
        catalog_.count=0;
        int result=cloud_load_m3u(&catalog_,(dir_+"playlist.m3u").c_str());
        if(result==-2) notice_="Lista limitada aos primeiros "+number(catalog_.count)+" itens.";
        else if(result) {catalog_.count=0;notice_="Coloque playlist.m3u junto ao default.xex e pressione Y.";}
        else if (!catalog_.count) notice_="A playlist não contém itens M3U com EXTINF.";
    } else if (section_==Files && page_!=Home) {
        if (!ReadFolder(folders_.back(),files_)) notice_="Não foi possível ler esta pasta.";
    } else if ((section_==Apps || section_==Emulators) && page_!=Home) {
        std::vector<std::string> roots;
        for(size_t i=0;i<roots_.size();++i) {
            size_t sep=roots_[i].find_last_of("\\/");
            roots.push_back(roots_[i].substr(0,sep+1)+(section_==Apps?"Apps":"Emulators"));
        }
        tools_=GameScanner(roots).Scan();
    } else games_=GameScanner(roots_).Scan();
    Rebuild();
}
void ConsoleHome::Enter(Section s) {
    section_=s; page_=Listing; row_=0; tab_=0;
    if (s==Files) {folders_.clear(); folders_.push_back(dir_);}
    Reload();
}
void ConsoleHome::LaunchFailed() { notice_="O console não iniciou o título. Confira se o dispositivo está conectado."; }

std::string ConsoleHome::Press(Key key) {
    if (key==Back) {
        notice_.clear();
        if (page_==Details) {
            page_=Listing;
            if(section_==Recent) { Rebuild(); row_=0; }
        }
        else if (page_==Listing && section_==Files && folders_.size()>1) {
            folders_.pop_back(); row_=0; Reload();
        } else page_=Home;
        return "";
    }
    if(key==Refresh) {
        if(page_==Details) page_=Listing;
        Reload();
        if(notice_.empty()) notice_="Atualizado";
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
    if(page_==Details) {
        if(key==Accept && section_!=Iptv && section_!=Files && !visible_.empty())
            return visible_[row_]->executable;
    } else {
        if(section_==Iptv && (key==Left || key==Right)) {
            tab_=(tab_+(key==Left?4:1))%5; row_=0; Rebuild();
        }
        if(key==Up && row_>0) --row_;
        if(key==Down && size_t(row_+1)<Count()) ++row_;
    }
    if(!Count() || section_==System) return "";
    if(key==Favorite && section_==Iptv) {
        const std::string path=dir_+"favorites.txt";
        if (cloud_favorite_toggle(&channelFavorites_,&channels_[filtered_[row_]])<0)
            notice_="Limite de favoritos atingido.";
        else if (cloud_favorites_save(&channelFavorites_,(path+".tmp").c_str())!=0 || !replaceSavedFile(path))
            notice_="Favorito desta sessão. Falha ao gravar no dispositivo.";
        if(tab_==CLOUD_FAVORITES) page_=Listing;
        Rebuild();
    } else if(key==Favorite && (section_==Library || section_==Recent || section_==Favorites)) {
        const std::string path=visible_[row_]->executable;
        std::vector<std::string>::iterator found=std::find(favorites_.begin(),favorites_.end(),path);
        if(found==favorites_.end()) favorites_.push_back(path); else favorites_.erase(found);
        if(!SaveProfile()) notice_="Favorito desta sessão. Falha ao gravar no dispositivo.";
        if(section_==Favorites) page_=Listing;
        Rebuild();
    } else if(key==Accept && page_==Listing) {
        if(section_==Files && files_[row_].directory) {
            const std::string& here=folders_.back();
            const char slash=dir_[dir_.size()-1];
            const std::string child=here+(here[here.size()-1]==slash?"":std::string(1,slash))+files_[row_].name;
            folders_.push_back(child);row_=0;Reload();
        } else {
            page_=Details;
            if(section_==Library || section_==Favorites || section_==Recent) {
                const std::string path=visible_[row_]->executable;
                history_.erase(std::remove(history_.begin(),history_.end(),path),history_.end());
                history_.insert(history_.begin(),path);
                if(history_.size()>HistoryLimit) history_.resize(HistoryLimit);
                if(!SaveProfile()) notice_="Histórico desta sessão. Falha ao gravar no dispositivo.";
            }
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

void ConsoleHome::DrawSection(HomeCanvas& c) const {
    c.Text(44,78,labels[section_],43,White,1190);
    if(section_==System) {
        c.Text(49,184,"Saída de vídeo   1280 x 720",27,White,1100);
        c.Text(49,248,"Jogos encontrados   "+number(games_.size()),25,Muted,1100);
        c.Text(49,310,"Catálogo Cloud360   "+number(catalog_.count)+" itens carregados",25,Muted,1100);
        return;
    }
    if(page_==Details && Count()) {
        c.Box(44,169,1190,329,Panel,12);
        std::string title,sub,hint;
        if(section_==Iptv) {
            const CloudItem& it=channels_[filtered_[row_]];
            title=it.name; sub=std::string(it.group)+"   /   "+cloud_format_label(cloud_format(it.url));
            hint="Catálogo disponível. Reprodução de áudio e vídeo no console em desenvolvimento.";
        } else if(section_==Files) {
            title=files_[row_].name; sub=folders_.back(); hint="Visualização de arquivos";
        } else {
            title=visible_[row_]->name; sub=visible_[row_]->executable; hint="Pressione A para iniciar este título.";
        }
        c.Text(76,219,title,36,White,1110);
        c.Text(77,298,sub,22,Muted,1100);
        c.Text(77,384,hint,21,Lime,1110);
        return;
    }
    if(section_==Iptv) {
        int x=46;
        for(int i=0;i<5;++i) {
            c.Box(x,151,155,45,i==tab_?0xff355c32u:Panel,6);
            c.Text(x+12,158,channelTabs[i],19,i==tab_?Lime:Muted,139);
            x+=166;
        }
    } else if(section_==Files) c.Text(49,157,folders_.back(),21,Muted,1140);
    else c.Text(49,157,number(Count())+" títulos",21,Muted,1140);
    if(!Count() && notice_.empty()) c.Text(56,246,"Nenhum item nesta seção.",25,Muted,1120);
    int start=row_/7*7;
    for(int i=start;i<static_cast<int>(Count()) && i<start+7;++i) {
        int y=212+(i-start)*59;
        panel(c,45,y,1189,52,Panel,i==row_);
        std::string name,detail;
        if(section_==Iptv) {
            const CloudItem& item=channels_[filtered_[i]];
            name=item.name; detail=std::string(cloud_favorite_has(&channelFavorites_,&item)?"*  ":"")+item.group;
        } else if(section_==Files) {
            name=files_[i].name; detail=files_[i].directory?"Pasta":"Arquivo";
        } else {name=visible_[i]->name; detail=(Starred(*visible_[i])?"*  ":"")+visible_[i]->device;}
        c.Text(62,y+6,name,23,White,810);
        c.Text(886,y+9,detail,19,Muted,336);
    }
}
void ConsoleHome::Draw(HomeCanvas& c,const std::string& clock) const {
    c.Background();
    c.Image(814,0,64,64,44,17,46,46); c.Text(103,16,"AURORA",32,White,145);
    c.Text(240,16,"X",32,Lime,44);
    c.Text(1088,19,clock,26,White,90);
    c.Box(1189,15,49,49,Lime,24); c.Box(1192,18,43,43,Panel,22);
    c.Text(1199,25,"AX",19,White,32);
    if(page_==Home) DrawHome(c); else DrawSection(c);
    if(!notice_.empty()) {
        c.Box(40,628,1200,35,0xff102923u,6);
        c.Text(51,630,notice_,18,Lime,1170);
    }
    c.Box(0,663,1280,57,0xff061915u);
    buttonHint(c,45,"A","Selecionar",0xff428914u);
    buttonHint(c,254,"B","Voltar",0xffb83c28u);
    buttonHint(c,429,"X","Favorito",0xff117db1u);
    buttonHint(c,632,"Y","Atualizar",0xffb3820du);
}
