#pragma once
#include "MediaUrl.h"
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>
#include <memory>
#include <functional>
#include <cstdint>

namespace CloudHls {
constexpr size_t MaxPlaylist=128*1024,MaxSegments=4096,MaxSegment=32*1024*1024;
struct Variant {std::string url;unsigned bandwidth;};
struct List {std::vector<Variant> variants;std::vector<std::string> segments;bool ended=false,hasSequence=false;uint64_t sequence=0;unsigned target=0;};
inline std::string Lower(std::string s){for(char& c:s)if(c>='A'&&c<='Z')c+=32;return s;}
inline bool Extension(const std::string& url,const std::string& ext){
    MediaUrl m;if(!m.Parse(url))return false;
    auto p=Lower(m.target.substr(0,m.target.find('?')));
    return p.size()>=ext.size()&&p.compare(p.size()-ext.size(),ext.size(),ext)==0;
}
// Restrict each resource to the original scheme/host/port. No redirects,
// scheme-relative URLs, dot-segments or fragments in this initial subset.
inline bool Resolve(const std::string& base,const std::string& uri,std::string& out){
    MediaUrl b,u;if(!b.Parse(base)||uri.empty()||uri.size()>2048||uri.find('#')!=std::string::npos||uri.compare(0,2,"//")==0)return false;
    if(uri.find("://")!=std::string::npos)out=uri;
    else{
        if(uri.find(':')!=std::string::npos||uri.front()=='?')return false;
        std::string root=(b.secure?"https://":"http://")+b.host;
        if(b.port!=(b.secure?443:80))root+=":"+std::to_string(b.port);
        const auto path=b.target.substr(0,b.target.find('?'));
        out=root+(uri.front()=='/'?uri:path.substr(0,path.find_last_of('/')+1)+uri);
    }
    if(!u.Parse(out)||u.secure!=b.secure||u.port!=b.port||Lower(u.host)!=Lower(b.host))return false;
    const auto path=u.target.substr(0,u.target.find('?'));
    if(path.find('%')!=std::string::npos)return false;
    std::istringstream parts(path);std::string part;
    while(std::getline(parts,part,'/'))if(part=="."||part=="..")return false;
    return true;
}
inline unsigned Bandwidth(const std::string& attrs){
    // Parse attributes, respecting quoted comma-separated CODECS.
    std::string field;bool quoted=false;unsigned result=0;
    for(size_t i=0;i<=attrs.size();++i){
        const char c=i==attrs.size()?',':attrs[i];
        if(c=='"')quoted=!quoted;
        if(c==','&&!quoted){
            if(field.compare(0,10,"BANDWIDTH=")==0){
                if(result||field.size()==10||field.size()>19)return 0;
                for(char x:field.substr(10)){if(x<'0'||x>'9')return 0;result=result*10+x-'0';}
                if(!result||result>100000000)return 0;
            }
            field.clear();
        }else field+=c;
    }
    return quoted?0:result;
}
inline bool Number(const std::string& text,uint64_t limit,uint64_t& value){
    value=0;if(text.empty())return false;
    for(char c:text){if(c<'0'||c>'9'||value>(limit-uint64_t(c-'0'))/10)return false;value=value*10+c-'0';}
    return value<=limit;
}
inline bool Parse(const std::string& base,const std::string& text,List& result,bool allowLive=false){
    result=List{};
    if(text.empty()||text.size()>MaxPlaylist)return false;
    std::istringstream in(text);std::string line;
    if(!std::getline(in,line))return false;
    if(!line.empty()&&line.back()=='\r')line.pop_back();
    if(line!="#EXTM3U")return false;
    unsigned bandwidth=0;bool duration=false;
    while(std::getline(in,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty())continue;
        if(line.size()>4096||result.ended)return false;
        if(line.compare(0,18,"#EXT-X-STREAM-INF:")==0){
            if(bandwidth||duration||!result.segments.empty())return false;
            bandwidth=Bandwidth(line.substr(18));if(!bandwidth)return false;
        }else if(line.compare(0,8,"#EXTINF:")==0){
            if(duration||bandwidth||!result.variants.empty())return false;
            const auto comma=line.find(',',8);
            if(comma==std::string::npos||comma==8)return false;
            bool digit=false,dot=false;double value=0,scale=1;
            for(size_t i=8;i<comma;++i){
                char c=line[i];if(c=='.'&&!dot){dot=true;continue;}
                if(c<'0'||c>'9')return false;digit=true;
                if(dot){scale*=10;value+=(c-'0')/scale;}else value=value*10+c-'0';
                if(value>120)return false;
            }
            if(!digit||value<=0)return false;
            duration=true;
        }else if(line.compare(0,22,"#EXT-X-MEDIA-SEQUENCE:")==0){
            uint64_t value=0;
            if(result.hasSequence||duration||!result.segments.empty()||!result.variants.empty()||!Number(line.substr(22),1000000000000ULL,value))return false;
            result.sequence=value;result.hasSequence=true;
        }else if(line.compare(0,22,"#EXT-X-TARGETDURATION:")==0){
            uint64_t value=0;
            if(result.target||duration||!result.segments.empty()||!Number(line.substr(22),120,value)||!value)return false;
            result.target=static_cast<unsigned>(value);
        }else if(line=="#EXT-X-ENDLIST"){
            if(bandwidth||duration||!result.variants.empty())return false;result.ended=true;
        }else if(line.compare(0,11,"#EXT-X-KEY:")==0){
            if(line!="#EXT-X-KEY:METHOD=NONE")return false;
        }else if(line[0]=='#'){
            if(line.compare(0,7,"#EXT-X-")==0&&
               line.compare(0,15,"#EXT-X-VERSION:")!=0&&
               line.compare(0,22,"#EXT-X-TARGETDURATION:")!=0&&
               line.compare(0,22,"#EXT-X-MEDIA-SEQUENCE:")!=0&&
               line.compare(0,21,"#EXT-X-PLAYLIST-TYPE:")!=0&&
               line!="#EXT-X-INDEPENDENT-SEGMENTS")return false;
        }else{
            std::string url;if(!Resolve(base,line,url))return false;
            if(bandwidth){
                if(result.variants.size()>=32||!Extension(url,".m3u8"))return false;
                result.variants.push_back({url,bandwidth});bandwidth=0;
            }else if(duration){
                if(result.segments.size()>=MaxSegments||!Extension(url,".ts"))return false;
                result.segments.push_back(url);duration=false;
            }else return false;
        }
    }
    return !bandwidth&&!duration&&((!result.variants.empty()&&result.segments.empty())||
        ((result.ended||(allowLive&&result.hasSequence&&result.target))&&!result.segments.empty()&&result.variants.empty()));
}
inline std::string Choose(const List& list,unsigned cap){
    const Variant* best=nullptr;
    for(const auto& v:list.variants)if(v.bandwidth<=cap&&(!best||v.bandwidth>best->bandwidth))best=&v;
    return best?best->url:"";
}
// Sequential VOD or opt-in live TS. A sliding window must preserve sequence
// identity. Never replay a segment after any bytes were delivered.
template<class Reader> class VodReader {
public:
    using Open=std::function<std::unique_ptr<Reader>(const std::string&,bool)>;
    using Wait=std::function<bool(unsigned)>; // milliseconds; false cancels
    explicit VodReader(Open open,Wait wait={}):open_(std::move(open)),wait_(std::move(wait)){}
    bool Start(const std::string& url,unsigned cap,bool allowLive=false){
        if(started_)return false;started_=true;allowLive_=allowLive;
        if(!Extension(url,".m3u8"))return false;
        playlist_=url;
        List list;if(!Fetch(playlist_,list))return false;
        if(!list.variants.empty()){
            playlist_=Choose(list,cap);
            if(playlist_.empty()||!Fetch(playlist_,list)||!list.variants.empty())return false;
        }
        if(list.segments.empty()||(!list.ended&&(!allowLive_||!wait_)))return false;
        next_=list.sequence;
        if(!list.ended&&list.segments.size()>3)next_+=list.segments.size()-3;
        list_=std::move(list);valid_=true;return true;
    }
    int Read(unsigned char* buffer,unsigned capacity){
        if(failed_||!valid_||!buffer||!capacity)return -1;
        unsigned refreshes=0,attempts=0;
        for(;;){
            const uint64_t end=list_.sequence+list_.segments.size();
            if(next_>=end){
                if(list_.ended)return 0;
                if(++refreshes>3||!wait_||!wait_(std::min(5000u,std::max(500u,list_.target*500u))))return Fail();
                List updated;
                if(!Fetch(playlist_,updated))continue;
                if(!Accept(updated))return Fail();
                continue;
            }
            if(next_<list_.sequence)return Fail();
            if(!reader_){
                reader_=open_(list_.segments[static_cast<size_t>(next_-list_.sequence)],false);
                bytes_=0;
                if(!reader_){
                    if(++attempts<=2&&wait_&&wait_(250u*attempts))continue;
                    return Fail();
                }
            }
            int n=reader_->Read(buffer,capacity);
            if(n<0){
                if(!bytes_&&++attempts<=2&&wait_&&wait_(250u*attempts)){reader_.reset();continue;}
                return Fail();
            }
            if(static_cast<unsigned>(n)>capacity||static_cast<size_t>(n)>MaxSegment-bytes_)return Fail();
            if(n){bytes_+=n;return n;}
            if(!bytes_||bytes_%188)return Fail();
            reader_.reset();++next_;attempts=0;
        }
    }
private:
    bool Accept(List& updated){
        if(!updated.variants.empty()||updated.segments.empty()||!updated.hasSequence||
           updated.sequence<list_.sequence||updated.sequence>next_||
           updated.sequence+updated.segments.size()<next_)return false;
        const uint64_t overlapEnd=std::min(list_.sequence+list_.segments.size(),updated.sequence+updated.segments.size());
        for(uint64_t seq=updated.sequence;seq<overlapEnd;++seq)
            if(list_.segments[static_cast<size_t>(seq-list_.sequence)]!=updated.segments[static_cast<size_t>(seq-updated.sequence)])return false;
        list_=std::move(updated);return true;
    }
    bool Fetch(const std::string& url,List& list){
        auto reader=open_(url,true);if(!reader)return false;
        std::string text;unsigned char block[4096];
        for(;;){int n=reader->Read(block,sizeof block);if(n<0||n>int(sizeof block)||size_t(n)>MaxPlaylist-text.size())return false;if(!n)break;text.append(reinterpret_cast<char*>(block),n);}
        return Parse(url,text,list,allowLive_);
    }
    int Fail(){failed_=true;reader_.reset();return -1;}
    Open open_;Wait wait_;std::unique_ptr<Reader> reader_;
    std::string playlist_;List list_;
    uint64_t next_=0;size_t bytes_=0;
    bool started_=false,failed_=false,valid_=false,allowLive_=false;
};
}
