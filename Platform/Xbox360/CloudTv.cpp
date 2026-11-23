#include "CloudTv.h"
#include "GameScanner.h"
#include "SavedFile.h"
#include <algorithm>
#include <ctime>
#include <set>
#include <sstream>

namespace {
const char* const tabs[]={"Todos","TV ao vivo","Filmes","Séries","Animes","Desenhos","Favoritos","Fontes"};
const unsigned Ink=0xffeff6eeu, Soft=0xffb1c8bbu, Lime=0xffa7ed48u, Panel=0xff153127u, Edge=0xff365344u;
const size_t PageSize=5;
const char letters[]="ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
std::string searchText(const std::string& text) {
    std::string result;result.reserve(text.size());
    for(size_t i=0;i<text.size();++i) {
        unsigned char c=static_cast<unsigned char>(text[i]);
        if(c==0xc3 && i+1<text.size()) {
            const unsigned char mark=static_cast<unsigned char>(text[++i])&0x1f;
            if(mark<=5) c='a';else if(mark==7) c='c';else if(mark<=11 && mark>=8) c='e';
            else if(mark>=12 && mark<=15) c='i';else if(mark==17) c='n';
            else if(mark>=18 && mark<=22) c='o';else if(mark>=25 && mark<=28) c='u';
            else c='?';
        }
        result+=c>='A' && c<='Z' ? char(c-'A'+'a') : char(c);
    }
    return result;
}
std::string countText(size_t n) { std::ostringstream s;s<<n;return s.str(); }
std::string localTime(int64_t timestamp) {
    time_t t=static_cast<time_t>(timestamp);const struct tm* local=std::localtime(&t);
    char text[16];return local && std::strftime(text,sizeof text,"%H:%M",local) ? text : "--:--";
}
void tile(HomeCanvas& c,int x,int y,int w,int h,bool focused) {
    c.Box(x,y,w,h,focused?Lime:Edge,8);int i=focused?3:1;
    c.Box(x+i,y+i,w-2*i,h-2*i,focused?0xff254831u:Panel,6);
}
void filesIn(const std::string& dir,const std::string& prefix,std::vector<std::string>& found) {
    std::vector<FolderEntry> items;if(!ReadFolder(dir+prefix,items)) return;
    for(size_t i=0;i<items.size();++i) if(!items[i].directory &&
        (tvEndsWith(items[i].name,".m3u") || tvEndsWith(items[i].name,".m3u8")))
        found.push_back(prefix+items[i].name);
}
void moveRow(size_t& row,size_t size,MenuKey key) {
    if(!size) {row=0;return;}
    const size_t step=(key==MenuPageUp || key==MenuPageDown)?PageSize:1;
    if(key==MenuUp || key==MenuPageUp) row=row>=step?row-step:0;
    if(key==MenuDown || key==MenuPageDown) row=std::min(row+step,size-1);
}
std::string protocol(const TvItem& item,const std::string& source) {
    if(!tvLocalMovie(source,item.uri).empty()) return "Arquivo WMV no dispositivo";
    const CloudFormat f=cloud_format(item.uri.c_str());
    if(f!=CLOUD_UNKNOWN) return std::string("Fonte de rede  /  ")+cloud_format_label(f);
    const size_t colon=item.uri.find(':');
    if(colon==std::string::npos) return "Arquivo local";
    const std::string scheme=tvLower(item.uri.substr(0,colon));
    if(scheme=="brazucaplay") return "Referência do Nexus";
    if(scheme=="game" || scheme=="hdd1" || scheme=="usb0" || scheme=="usb1") return "Arquivo local";
    return "Referência de catálogo";
}
struct EpisodeOrder {
    const std::vector<TvItem>& items;
    explicit EpisodeOrder(const std::vector<TvItem>& data):items(data) {}
    bool operator()(size_t a,size_t b) const {
        TvEpisode left,right;tvEpisode(items[a].name,left);tvEpisode(items[b].name,right);
        return left.episode<right.episode;
    }
};
}
CloudTv::CloudTv(const std::string& dir):dir_(dir),row_(0),episodeRow_(0),season_(0),detail_(0),
    tab_(TvAll),view_(Browse),behindDetails_(Browse),behindEpisodes_(Browse),guideOffset_(0),letter_(0) {favorites_.count=0;}
void CloudTv::Discover() {
    sources_.clear();filesIn(dir_,"",sources_);
    const char slash=(dir_.find('\\')!=std::string::npos)?'\\':'/';
    filesIn(dir_,std::string("Listas")+slash,sources_);
    std::sort(sources_.begin(),sources_.end());
}
bool CloudTv::Load(const std::string& source) {
    if(source.empty() || !catalog_.Load(dir_+source)) {
        notice_="Não foi possível abrir a lista. Confira o dispositivo e as fontes.";return false;
    }
    source_=source; const size_t dot=source.find_last_of('.');
    const std::string paired=source.substr(0,dot)+".xml";
    const TvGuide::Status state=guide_.Load(dir_+(fileExists(dir_+paired)?paired:"guide.xml"));
    notice_.clear();
    if(catalog_.Limited()) notice_="Catálogo parcial. O limite de itens ou de memória foi atingido.";
    else if(state==TvGuide::Invalid) notice_="A lista foi carregada. Confira a sintaxe do XMLTV.";
    else if(state==TvGuide::Limited) notice_="Guia parcial. Foram lidos os primeiros 65.536 programas.";
    else if(!Count()) notice_="A lista está vazia ou não contém entradas EXTINF.";
    return true;
}
void CloudTv::Open() {
    row_=0;tab_=TvAll;view_=Browse;pending_.clear();notice_.clear();query_.clear();guideOffset_=0;
    favorites_.count=0;cloud_favorites_load(&favorites_,savedFile(dir_+"favorites.txt").c_str());
    Discover();
    std::string chosen;std::ifstream in(savedFile(dir_+"cloud360-source.txt").c_str());std::getline(in,chosen);
    if(!chosen.empty() && chosen[chosen.size()-1]=='\r') chosen.erase(chosen.size()-1);
    if(std::find(sources_.begin(),sources_.end(),chosen)==sources_.end()) {
        chosen="";
        for(size_t i=0;i<sources_.size();++i) if(tvLower(sources_[i])=="playlist.m3u") chosen=sources_[i];
        if(chosen.empty() && !sources_.empty()) chosen=sources_.front();
    }
    // Entering again must not expose an old catalogue from a disconnected device.
    if(!Load(chosen)) {
        catalog_=TvCatalog();guide_=TvGuide();source_.clear();
        if(sources_.empty()) notice_="Copie uma lista M3U para a pasta da dashboard ou para Listas e pressione Y.";
    }
    Filter();
}
void CloudTv::Filter() {
    rows_.clear();std::set<std::string> shows;
    const std::vector<TvItem>& items=catalog_.Items();
    const std::string needle=searchText(query_);
    if(tab_!=TvSources) {
        std::set<uint64_t> marked;
        if(tab_==TvFavorites) marked.insert(favorites_.ids,favorites_.ids+favorites_.count);
        for(size_t i=0;i<items.size();++i) {
            if(tab_==TvFavorites ? marked.count(items[i].id)==0 : tab_!=TvAll && items[i].category!=tab_) continue;
            if(!needle.empty() && searchText(items[i].name).find(needle)==std::string::npos &&
               searchText(items[i].group).find(needle)==std::string::npos) continue;
            if(tab_==TvSeries || tab_==TvAnime || tab_==TvCartoon) {
                TvEpisode ep;
                if(tvEpisode(items[i].name,ep) && !shows.insert(tvLower(ep.show)).second) continue;
            }
            rows_.push_back(i);
        }
    }
    const size_t count=tab_==TvSources?sources_.size():rows_.size();
    if(row_>=count) row_=count?count-1:0;
}
void CloudTv::OpenItem(size_t index) {
    detail_=index;const TvItem& item=catalog_.Items()[index];TvEpisode ep;
    if((item.category==TvSeries || item.category==TvAnime || item.category==TvCartoon) && tvEpisode(item.name,ep)) {
        show_=ep.show;const std::string showKey=tvLower(show_);std::set<unsigned> seasons;
        for(size_t i=0;i<Count();++i) {
            const TvItem& other=catalog_.Items()[i];
            if(other.category==item.category && tvEpisode(other.name,ep) && tvLower(ep.show)==showKey) seasons.insert(ep.season);
        }
        seasons_.assign(seasons.begin(),seasons.end());season_=0;episodeRow_=0;behindEpisodes_=view_;
        const bool specific=tab_==TvAll || tab_==TvFavorites || !query_.empty();
        if(specific && tvEpisode(item.name,ep))
            season_=std::find(seasons_.begin(),seasons_.end(),ep.season)-seasons_.begin();
        view_=Episodes;CollectEpisodes();
        if(specific) episodeRow_=std::find(episodes_.begin(),episodes_.end(),index)-episodes_.begin();
    } else {behindDetails_=view_;view_=Details;}
}
void CloudTv::CollectEpisodes() {
    episodes_.clear();const std::vector<TvItem>& items=catalog_.Items();
    const std::string key=tvLower(show_);const TvCategory cat=items[detail_].category;
    for(size_t i=0;i<items.size();++i) {
        TvEpisode ep;
        if(items[i].category==cat && tvEpisode(items[i].name,ep) && ep.season==seasons_[season_] && tvLower(ep.show)==key)
            episodes_.push_back(i);
    }
    std::stable_sort(episodes_.begin(),episodes_.end(),EpisodeOrder(items));
}
void CloudTv::Toggle(size_t index) {
    if(cloud_favorite_toggle_id(&favorites_,catalog_.Items()[index].id)<0) {notice_="Limite de favoritos atingido.";return;}
    const std::string file=dir_+"favorites.txt";
    if(cloud_favorites_save(&favorites_,(file+".tmp").c_str())!=0 || !replaceSavedFile(file))
        notice_="Favorito desta sessão. Falha ao gravar no dispositivo.";
    if(tab_==TvFavorites) {view_=Browse;Filter();}
}
std::string CloudTv::TakeMovie() {std::string path;path.swap(pending_);return path;}
bool CloudTv::Press(MenuKey key) {
    notice_.clear();
    if(view_==Search) {SearchKey(key);return true;}
    if(key==MenuSearch && view_==Browse && tab_!=TvSources) {
        edit_=query_;letter_=0;view_=Search;return true;
    }
    if(key==MenuBack) {
        if(view_==Browse) return false;
        if(view_==Details) view_=behindDetails_;
        else if(view_==Episodes) view_=behindEpisodes_;
        else view_=Browse;
        return true;
    }
    if(key==MenuRefresh) {
        const TvCategory last=tab_;const std::string query=query_;Open();tab_=last;query_=query;Filter();return true;
    }
    if(key==MenuGuide && (view_==Browse || view_==Guide)) {
        if(view_==Guide) view_=Browse;
        else {if(tab_!=TvLive) row_=0;tab_=TvLive;view_=Guide;guideOffset_=0;Filter();}
        return true;
    }
    if(view_==Details) {
        if(key==MenuFavorite) Toggle(detail_);
        if(key==MenuAccept) {
            pending_=tvLocalMovie(dir_+source_,catalog_.Items()[detail_].uri);
            if(pending_.empty()) notice_="Este item ainda não tem reprodução no console.";
        }
        return true;
    }
    if(view_==Episodes) {
        if(key==MenuLeft || key==MenuRight) {
            season_=(season_+(key==MenuLeft?seasons_.size()-1:1))%seasons_.size();
            episodeRow_=0;CollectEpisodes();
        } else if(!episodes_.empty()) {
            moveRow(episodeRow_,episodes_.size(),key);
            if(key==MenuAccept) {detail_=episodes_[episodeRow_];behindDetails_=Episodes;view_=Details;}
            if(key==MenuFavorite) Toggle(episodes_[episodeRow_]);
        }
        return true;
    }
    if(key==MenuLeft || key==MenuRight) {
        if(view_==Guide) guideOffset_=std::max(-24,std::min(48,guideOffset_+(key==MenuLeft?-1:1)));
        else {tab_=static_cast<TvCategory>((tab_+(key==MenuLeft?TvTabCount-1:1))%TvTabCount);row_=0;Filter();}
        return true;
    }
    const size_t count=tab_==TvSources?sources_.size():rows_.size();
    if(!count) return true;
    moveRow(row_,count,key);
    if(tab_==TvSources) {
        if(key==MenuAccept && Load(sources_[row_])) {
            const std::string file=dir_+"cloud360-source.txt";
            std::ofstream out((file+".tmp").c_str());out<<source_<<"\n";out.close();
            if(!out || !replaceSavedFile(file)) notice_="Lista ativa nesta sessão. Falha ao salvar a escolha.";
            tab_=TvAll;row_=0;Filter();
        }
    } else if(key==MenuFavorite) Toggle(rows_[row_]);
    else if(key==MenuAccept) OpenItem(rows_[row_]);
    return true;
}

void CloudTv::DrawArt(HomeCanvas& c,size_t index,int x,int y,int w,int h) const {
    const std::string slash=dir_.find('\\')!=std::string::npos?"\\":"/";
    const size_t dot=source_.find_last_of('.');
    const std::string file=countText(index)+".rgba";
    const std::string pack=dir_+source_.substr(0,dot)+".logos"+slash+file;
    c.Box(x,y,w,h,0xff0c2821u,6);
    const int aw=std::min(w,h*16/9),ah=aw*9/16,ax=x+(w-aw)/2,ay=y+(h-ah)/2;
    if(c.Logo(pack,ax,ay,aw,ah)) return;
    if(tvLower(source_)=="playlist.m3u" && c.Logo(dir_+"logos"+slash+file,ax,ay,aw,ah)) return;
    const TvItem& item=catalog_.Items()[index];
    if(item.category==TvLive) {
        c.Image(344,0,254,222,x+(w-h)/2,y+3,h,h-6);
        return;
    }
    // A filmstrip marks entries whose playlist does not supply an offline cover.
    const int fw=std::min(w-30,2*h),fh=std::min(h-20,fw/2),fx=x+(w-fw)/2,fy=y+(h-fh)/2;
    c.Box(fx,fy,fw,fh,0xff31533du,7);
    for(int i=12;i<fw-10;i+=23) {
        c.Box(fx+i,fy+5,13,7,0xff0c2119u,2);c.Box(fx+i,fy+fh-12,13,7,0xff0c2119u,2);
    }
    c.Box(fx+12,fy+18,fw-24,fh-36,0xff193928u,4);
    c.Text(fx+28,fy+fh/2-20,tabs[item.category],31,Lime,fw-52);
}
void CloudTv::DrawList(HomeCanvas& c,int64_t now) const {
    const bool picking=tab_==TvSources;
    c.Text(44,76,picking?"Fontes de conteúdo":tab_==TvAll?"Cloud360 IPTV":tab_==TvLive?"TV ao vivo":
           tab_==TvFavorites?"Favoritos":"Cinema",40,Ink,750);
    const size_t total=picking?sources_.size():rows_.size();
    if(!query_.empty() && !picking) c.Text(723,98,"Busca  "+query_,17,Lime,215);
    c.Text(944,98,countText(total)+(picking?" listas":" itens"),19,Soft,280);
    for(int i=0;i<TvTabCount;++i) {
        const int x=45+i*150;
        c.Box(x,146,143,42,i==tab_?Lime:Panel,6);
        c.Text(x+12,152,tabs[i],19,i==tab_?0xff102312u:Soft,122);
    }
    if(!total) {
        c.Text(58,250,picking?"Nenhuma lista encontrada.":"Nenhum item nesta seção.",26,Soft,1130);
        if(picking)c.Text(58,300,"Coloque um M3U na pasta da dashboard ou na subpasta Listas.",19,Soft,1100);
        return;
    }
    const size_t first=row_/PageSize*PageSize;
    for(size_t i=first;i<total && i<first+PageSize;++i) {
        const int y=210+int(i-first)*78;tile(c,46,y,559,70,i==row_);
        std::string label,subtitle;
        if(picking) {label=sources_[i];subtitle=label==source_?"Lista ativa":"Lista local";}
        else {
            const TvItem& item=catalog_.Items()[rows_[i]];
            label=item.name;subtitle=item.group;
            TvEpisode ep;
            if((tab_==TvSeries || tab_==TvAnime || tab_==TvCartoon) && tvEpisode(label,ep)) label=ep.show;
            if(item.category==TvLive) {
                const TvProgramme* current=TvGuide::Current(guide_.For(item.channel,item.name),now);
                if(current) subtitle=current->title;
            }
            if(cloud_favorite_has_id(&favorites_,item.id)) subtitle="*  "+subtitle;
        }
        c.Text(62,y+4,label,23,Ink,520);c.Text(63,y+38,subtitle,16,Soft,510);
    }
    tile(c,624,210,610,385,false);
    if(picking) {
        c.Text(649,238,"Lista selecionada",29,Ink,554);
        c.Text(649,302,sources_[row_],23,Lime,552);
        c.Text(649,373,"A abre a lista e salva a escolha.",20,Soft,552);
        c.Text(649,429,"Guia XMLTV",21,Ink,550);
        c.Text(649,471,"Use um XML com o nome da lista ou guide.xml.",19,Soft,552);
    } else {
        const size_t index=rows_[row_];const TvItem& item=catalog_.Items()[index];
        DrawArt(c,index,636,221,585,202);
        c.Text(650,434,item.group,17,Lime,551);
        c.Text(650,463,item.name,28,Ink,550);
        if(item.category==TvLive) {
            const TvGuide::Schedule& list=guide_.For(item.channel,item.name);
            const TvProgramme* on=TvGuide::Current(list,now);
            const TvProgramme* next=TvGuide::Next(list,now);
            c.Text(650,516,on?"Agora   "+on->title:"Agora   Sem programação",18,Soft,551);
            c.Text(650,553,next?"A seguir   "+localTime(next->start)+"   "+next->title:"A seguir   Sem programação",17,Soft,548);
        } else c.Text(650,535,protocol(item,dir_+source_),19,Soft,550);
    }
    c.Text(48,602,countText(row_+1)+" / "+countText(total),15,Soft,300);
}
void CloudTv::DrawGuide(HomeCanvas& c,int64_t now) const {
    c.Text(45,78,"Guia de programação",40,Ink,720);
    const int left=310,w=920;
    const int64_t first=now-now%1800+int64_t(guideOffset_)*1800,end=first+5400;
    c.Text(49,162,"Canal",21,Lime,245);
    for(int j=0;j<3;++j) c.Text(left+j*w/3,162,localTime(first+1800*j),20,Soft,250);
    if(rows_.empty()) {c.Text(50,260,"Nenhum canal de TV nesta lista.",25,Soft,1100);return;}
    const size_t base=row_/PageSize*PageSize;
    for(size_t i=base;i<rows_.size() && i<base+PageSize;++i) {
        const TvItem& item=catalog_.Items()[rows_[i]];const int y=213+int(i-base)*76;
        tile(c,45,y,253,67,i==row_);c.Text(57,y+14,item.name,20,Ink,227);
        c.Box(left,y,w,67,Panel,4);
        const TvGuide::Schedule& list=guide_.For(item.channel,item.name);
        bool painted=false;
        for(size_t j=0;j<list.size() && list[j].start<end;++j) {
            const TvProgramme& prog=list[j];if(prog.stop<=first) continue;
            int x=int((std::max(first,prog.start)-first)*w/5400);
            int stop=int((std::min(end,prog.stop)-first)*w/5400);
            if(stop-x<3) continue;
            tile(c,left+x,y,stop-x-1,67,i==row_ && prog.start<=now && now<prog.stop);
            if(stop-x>35) c.Text(left+x+7,y+15,prog.title,18,Ink,stop-x-14);
            painted=true;
        }
        if(!painted)c.Text(left+15,y+17,"Sem programação",18,Soft,w-30);
    }
    if(now>=first && now<end)c.Box(left+int((now-first)*w/5400),202,2,int(std::min(PageSize,rows_.size()-base))*76,Lime);
    c.Text(47,601,"Esquerda e direita mudam o horário em 30 minutos.",16,Soft,1160);
}
void CloudTv::DrawDetails(HomeCanvas& c,int64_t now) const {
    const TvItem& item=catalog_.Items()[detail_];
    c.Text(45,80,item.category==TvLive?"Detalhes do canal":"Detalhes de mídia",38,Ink,1130);
    tile(c,45,171,1190,393,false);DrawArt(c,detail_,60,194,460,259);
    c.Text(550,195,item.group,19,Lime,655);c.Text(550,238,item.name,30,Ink,650);
    c.Text(550,303,protocol(item,dir_+source_),20,Soft,655);
    if(item.category==TvLive) {
        const TvGuide::Schedule& list=guide_.For(item.channel,item.name);
        const TvProgramme* on=TvGuide::Current(list,now);const TvProgramme* next=TvGuide::Next(list,now);
        c.Text(550,353,on?"Agora   "+on->title:"Agora   Sem programação",19,Ink,651);
        if(next)c.Text(550,398,"A seguir   "+localTime(next->start)+"   "+next->title,18,Soft,650);
    }
    if(!tvLocalMovie(dir_+source_,item.uri).empty()) {
        c.Box(550,470,212,52,Lime,6);c.Text(570,480,"Reproduzir WMV",21,0xff0c190fu,181);
    } else c.Text(550,467,"Reprodução indisponível para esta fonte no console.",18,Soft,650);
}
void CloudTv::DrawEpisodes(HomeCanvas& c) const {
    c.Text(45,78,"Temporadas e episódios",39,Ink,1160);
    tile(c,45,171,416,417,false);DrawArt(c,detail_,60,190,386,217);
    c.Text(63,427,"Série",17,Lime,370);c.Text(63,461,show_,27,Ink,370);
    c.Text(487,169,"Temporada "+countText(seasons_[season_]),25,Lime,713);
    const size_t base=episodeRow_/PageSize*PageSize;
    for(size_t i=base;i<episodes_.size() && i<base+PageSize;++i) {
        const TvItem& item=catalog_.Items()[episodes_[i]];TvEpisode ep;tvEpisode(item.name,ep);
        int y=221+int(i-base)*71;tile(c,483,y,750,62,i==episodeRow_);
        c.Text(497,y+13,countText(ep.episode),20,Lime,47);c.Text(556,y+11,item.name,20,Ink,660);
    }
    c.Text(488,591,"Esquerda e direita mudam a temporada.",17,Soft,705);
}
void CloudTv::Draw(HomeCanvas& c,int64_t now) const {
    if(view_==Search) {DrawSearch(c);return;}
    if(view_==Guide) DrawGuide(c,now);
    else if(view_==Episodes) DrawEpisodes(c);
    else if(view_==Details) DrawDetails(c,now);
    else DrawList(c,now);
    if(!notice_.empty()) {
        c.Box(44,626,1191,35,0xff132e23u,6);c.Text(57,629,notice_,18,Lime,1160);
    }
    c.Box(0,663,1280,57,0xff061915u);
    const bool playable=view_==Details && !tvLocalMovie(dir_+source_,catalog_.Items()[detail_].uri).empty();
    c.Text(44,675,view_==Details?(playable?"A  Reproduzir":""):"A  Selecionar",19,Lime,190);
    c.Text(240,675,"B  Voltar",19,Ink,130);
    if(tab_!=TvSources)c.Text(390,675,"X  Favorito",19,Ink,150);
    c.Text(567,675,"Y  Atualizar",19,Ink,151);
    if(view_!=Details && view_!=Episodes)c.Text(764,675,"Start  Guia",19,Ink,159);
    if(view_!=Details)c.Text(922,675,"LB/RB  Página",18,Ink,155);
    if(view_==Browse && tab_!=TvSources)c.Text(1092,675,"RS  Buscar",18,Ink,152);
}
void CloudTv::SearchKey(MenuKey key) {
    if(key==MenuBack) {view_=Browse;return;}
    if(key==MenuRefresh || key==MenuGuide || (key==MenuAccept && letter_==39)) {
        query_=edit_;row_=0;view_=Browse;Filter();return;
    }
    if(key==MenuFavorite || (key==MenuAccept && letter_==37)) {
        if(!edit_.empty()) edit_.erase(edit_.size()-1);
        return;
    }
    if(key==MenuAccept) {
        if(letter_==38) edit_.clear();
        else if(edit_.size()<32) edit_+=letter_==36?' ':letters[letter_];
    }
    if(key==MenuLeft && letter_%6>0) --letter_;
    if(key==MenuRight && letter_%6<5 && letter_<39) ++letter_;
    if(key==MenuUp && letter_>=6) letter_-=6;
    if(key==MenuDown && letter_<36) letter_=std::min(letter_+6,39);
}
void CloudTv::DrawSearch(HomeCanvas& c) const {
    c.Text(46,77,"Buscar no catálogo",38,Ink,1160);
    tile(c,48,147,1168,55,false);
    c.Text(66,155,edit_.empty()?"Digite o nome ou a categoria":edit_,23,edit_.empty()?Soft:Ink,1120);
    const char* actions[]={"Espaço","Apagar","Limpar","Buscar"};
    for(int i=0;i<40;++i) {
        int x=48+(i%6)*196,y=220+(i/6)*56;tile(c,x,y,183,49,i==letter_);
        c.Text(x+15,y+8,i<36?std::string(1,letters[i]):actions[i-36],20,i==letter_?Lime:Ink,155);
    }
    c.Box(0,663,1280,57,0xff061915u);
    c.Text(44,675,"A  Selecionar",19,Lime,203);c.Text(260,675,"B  Cancelar",19,Ink,205);
    c.Text(482,675,"X  Apagar",19,Ink,209);c.Text(706,675,"Y  Buscar",19,Ink,200);
}
