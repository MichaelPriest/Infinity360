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
struct List {std::vector<Variant> variants;std::vector<std::string> segments;bool ended=false;};
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
inline bool Parse(const std::string& base,const std::string& text,List& result){
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
        (result.ended&&!result.segments.empty()&&result.variants.empty()));
}
inline std::string Choose(const List& list,unsigned cap){
    const Variant* best=nullptr;
    for(const auto& v:list.variants)if(v.bandwidth<=cap&&(!best||v.bandwidth>best->bandwidth))best=&v;
    return best?best->url:"";
}
// Reader consumes exactly one segment at a time. No replay after bytes are
// delivered: errors are terminal to avoid duplicate TS packets/timestamps.
template<class Reader> class VodReader {
public:
    using Open=std::function<std::unique_ptr<Reader>(const std::string&,bool)>;
    explicit VodReader(Open open):open_(std::move(open)){}
    bool Start(const std::string& url,unsigned cap){
        if(started_)return false;started_=true;
        if(!Extension(url,".m3u8"))return false;
        List list;if(!Fetch(url,list))return false;
        if(!list.variants.empty()){
            auto selected=Choose(list,cap);if(selected.empty()||!Fetch(selected,list)||!list.variants.empty())return false;
        }
        if(!list.ended||list.segments.empty())return false;
        segments_=std::move(list.segments);return true;
    }
    int Read(unsigned char* buffer,unsigned capacity){
        if(failed_||!buffer||!capacity||segments_.empty())return -1;
        while(index_<segments_.size()){
            if(!reader_){reader_=open_(segments_[index_],false);bytes_=0;if(!reader_)return Fail();}
            int n=reader_->Read(buffer,capacity);
            if(n<0||static_cast<unsigned>(n)>capacity||static_cast<size_t>(n)>MaxSegment-bytes_)return Fail();
            if(n){bytes_+=n;return n;}
            if(!bytes_||bytes_%188)return Fail();
            reader_.reset();++index_;
        }
        return 0;
    }
private:
    bool Fetch(const std::string& url,List& list){
        auto reader=open_(url,true);if(!reader)return false;
        std::string text;unsigned char block[4096];
        for(;;){int n=reader->Read(block,sizeof block);if(n<0||n>int(sizeof block)||size_t(n)>MaxPlaylist-text.size())return false;if(!n)break;text.append(reinterpret_cast<char*>(block),n);}
        return Parse(url,text,list);
    }
    int Fail(){failed_=true;reader_.reset();return -1;}
    Open open_;std::unique_ptr<Reader> reader_;
    std::vector<std::string> segments_;
    size_t index_=0,bytes_=0;bool started_=false,failed_=false;
};
}
