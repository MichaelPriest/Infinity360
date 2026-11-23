#include "TvCatalog.h"
#include <algorithm>
#include <cctype>
#include <cstring>

std::string tvLower(std::string s) {
    for(size_t i=0;i<s.size();++i) if(s[i]>='A' && s[i]<='Z') s[i]=char(s[i]-'A'+'a');
    return s;
}
bool tvEndsWith(const std::string& value,const std::string& ending) {
    return value.size()>=ending.size() && tvLower(value.substr(value.size()-ending.size()))==ending;
}
namespace {
bool digit(char c) { return c>='0' && c<='9'; }
bool separator(char c) { return c==' ' || c=='.' || c=='_' || c=='-' || c=='[' || c=='(' || c==']' || c==')' || c==':'; }
bool digits(const std::string& s,size_t& pos,unsigned& value,unsigned limit) {
    size_t first=pos; value=0;
    while(pos<s.size() && digit(s[pos]) && pos-first<limit) value=value*10+unsigned(s[pos++]-'0');
    return pos>first && (pos==s.size() || !digit(s[pos]));
}
TvCategory categoryOf(const CloudItem& item) {
    const CloudTab kind=cloud_item_type(&item);
    if(kind==CLOUD_LIVE) return TvLive;
    const std::string group=tvLower(item.group);
    if(group.find("anime")!=std::string::npos) return TvAnime;
    if(group.find("desenho")!=std::string::npos) return TvCartoon;
    return kind==CLOUD_SERIES ? TvSeries : TvFilm;
}
}
bool tvEpisode(const std::string& title,TvEpisode& result) {
    const std::string s=tvLower(title);
    for(size_t i=1;i<s.size();++i) {
        if(!separator(s[i-1])) continue;
        size_t p=i; const bool marked=s[p]=='s' || s[p]=='t';
        if(marked) ++p;
        unsigned season,ep;
        if(!digits(s,p,season,2)) continue;
        if(marked) while(p<s.size() && (s[p]==' ' || s[p]=='.')) ++p;
        if(p==s.size() || s[p++]!=(marked?'e':'x') || !digits(s,p,ep,3)) continue;
        if(p<s.size() && !separator(s[p])) continue;
        size_t end=i; while(end && separator(s[end-1])) --end;
        if(!end) continue;
        result.show=title.substr(0,end); result.season=season; result.episode=ep;
        return true;
    }
    return false;
}
std::string tvLocalMovie(const std::string& playlist,const std::string& uri) {
    if(!tvEndsWith(uri,".wmv") || uri.find_first_of("?#*|\r\n")!=std::string::npos || uri.empty()) return "";
    std::string path=uri;
    std::replace(path.begin(),path.end(),'\\','/');
    const size_t colon=path.find(':');
    if(colon!=std::string::npos) {
        const std::string volume=tvLower(path.substr(0,colon));
        if((volume!="game" && volume!="hdd1" && volume!="usb0" && volume!="usb1") ||
           path.compare(colon,2,":/")!=0 || path.find(':',colon+1)!=std::string::npos) return "";
    } else {
        if(path[0]=='/') return "";
        size_t slash=playlist.find_last_of("/\\");
        path=(slash==std::string::npos?"":playlist.substr(0,slash+1))+path;
    }
    if(playlist.find('\\')!=std::string::npos || colon!=std::string::npos)
        std::replace(path.begin(),path.end(),'/','\\');
    return path;
}
TvCatalog::TvCatalog() : textBytes_(0),limited_(false) {}
int TvCatalog::Append(const CloudItem* raw,void* opaque) {
    TvCatalog& catalog=*static_cast<TvCatalog*>(opaque);
    size_t bytes=std::strlen(raw->name)+std::strlen(raw->group)+std::strlen(raw->url)+std::strlen(raw->tvg_id)+4;
    if(catalog.items_.size()>=65536 || catalog.textBytes_+bytes>20*1024*1024) {
        catalog.limited_=true;return 1;
    }
    TvItem item;
    item.name=raw->name; item.group=raw->group; item.uri=raw->url; item.channel=raw->tvg_id;
    item.id=cloud_item_id(raw); item.category=categoryOf(*raw);
    catalog.items_.push_back(item);catalog.textBytes_+=bytes;
    return 0;
}
bool TvCatalog::Load(const std::string& path) {
    TvCatalog next;
    if(cloud_visit_m3u(path.c_str(),Append,&next)<0) return false;
    items_.swap(next.items_);textBytes_=next.textBytes_;limited_=next.limited_;
    return true;
}
