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
constexpr size_t MaxPlaylist=128*1024,MaxSegments=4096,MaxSegment=32*1024*1024,MaxBufferedSegment=2*1024*1024;
struct Variant {std::string url;unsigned bandwidth=0,width=0,height=0;bool compatible=true;};
struct List {std::string init;std::vector<Variant> variants;std::vector<std::string> segments;bool ended=false,hasSequence=false;uint64_t sequence=0;unsigned target=0;};
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
inline bool Number(const std::string& text,uint64_t limit,uint64_t& value){
    value=0;if(text.empty())return false;
    for(char c:text){if(c<'0'||c>'9'||uint64_t(c-'0')>limit||value>(limit-uint64_t(c-'0'))/10)return false;value=value*10+c-'0';}
    return value<=limit;
}
constexpr unsigned DefaultBitrate=2000000,MinBitrate=100000,MaxBitrate=4000000;
inline bool BitrateCap(const std::string& text,unsigned& cap){
    uint64_t value=0;if(!Number(text,MaxBitrate,value)||value<MinBitrate)return false;
    cap=static_cast<unsigned>(value);return true;
}
inline bool SupportedCodecs(const std::string& value){
    if(value.size()<2||value.front()!='"'||value.back()!='"')return false;
    std::istringstream in(Lower(value.substr(1,value.size()-2)));
    std::string codec;bool video=false,audio=false;
    while(std::getline(in,codec,',')){
        if(codec=="avc1"||codec=="avc3"||codec.compare(0,5,"avc1.")==0||codec.compare(0,5,"avc3.")==0){
            if(video)return false;video=true;
        }else if(codec=="mp4a"||codec=="mp4a.40.2"||codec=="mp4a.40.5"||codec=="mp4a.40.29"){
            if(audio)return false;audio=true;
        }else return false;
    }
    return video&&audio;
}
// Unknown codec/resolution metadata is inconclusive, not verified. The decoder
// still independently enforces H.264/AAC, actual frame size and timestamps.
inline bool ParseVariant(const std::string& attrs,Variant& variant){
    variant=Variant{};
    std::string field;bool quoted=false,codecs=false,resolution=false;
    for(size_t i=0;i<=attrs.size();++i){
        const char c=i==attrs.size()?',':attrs[i];
        if(c=='"')quoted=!quoted;
        if(c==','&&!quoted){
            const auto eq=field.find('=');
            if(eq==std::string::npos||eq==0||eq+1==field.size())return false;
            const auto key=field.substr(0,eq),value=field.substr(eq+1);
            if(key=="BANDWIDTH"){
                uint64_t n=0;if(variant.bandwidth||!Number(value,100000000,n)||!n)return false;
                variant.bandwidth=static_cast<unsigned>(n);
            }else if(key=="RESOLUTION"){
                const auto x=value.find('x');uint64_t w=0,h=0;
                if(resolution||x==std::string::npos||!Number(value.substr(0,x),16384,w)||!Number(value.substr(x+1),16384,h)||!w||!h)return false;
                resolution=true;variant.width=static_cast<unsigned>(w);variant.height=static_cast<unsigned>(h);
                if(w>1280||h>720)variant.compatible=false;
            }else if(key=="CODECS"){
                if(codecs||value.front()!='"'||value.back()!='"')return false;
                codecs=true;if(!SupportedCodecs(value))variant.compatible=false;
            }else if(key=="AUDIO"||key=="VIDEO"||key=="SUBTITLES"){
                // Alternate renditions require separate synchronized streams.
                variant.compatible=false;
            }
            field.clear();
        }else field+=c;
    }
    return !quoted&&variant.bandwidth>0;
}
inline bool Parse(const std::string& base,const std::string& text,List& result,bool allowLive=false){
    result=List{};
    if(text.empty()||text.size()>MaxPlaylist)return false;
    std::istringstream in(text);std::string line;
    if(!std::getline(in,line))return false;
    if(!line.empty()&&line.back()=='\r')line.pop_back();
    if(line!="#EXTM3U")return false;
    Variant variant;bool duration=false;
    while(std::getline(in,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty())continue;
        if(line.size()>4096||result.ended)return false;
        if(line.compare(0,18,"#EXT-X-STREAM-INF:")==0){
            if(variant.bandwidth||duration||!result.segments.empty())return false;
            if(!ParseVariant(line.substr(18),variant))return false;
        }else if(line.compare(0,8,"#EXTINF:")==0){
            if(duration||variant.bandwidth||!result.variants.empty())return false;
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
        }else if(line.compare(0,16,"#EXT-X-MAP:URI=\"")==0){
            if(line.size()<18||!result.init.empty()||!result.segments.empty()||!result.variants.empty()||variant.bandwidth||duration||line.back()!='"')return false;
            const auto uri=line.substr(16,line.size()-17);
            if(!Resolve(base,uri,result.init)||!Extension(result.init,".mp4"))return false;
        }else if(line=="#EXT-X-ENDLIST"){
            if(variant.bandwidth||duration||!result.variants.empty())return false;result.ended=true;
        }else if(line.compare(0,11,"#EXT-X-KEY:")==0){
            if(line!="#EXT-X-KEY:METHOD=NONE")return false;
        }else if(line[0]=='#'){
            // These tags describe presentation/time, not segment decoding.
            if(line.compare(0,25,"#EXT-X-PROGRAM-DATE-TIME:")==0||line.compare(0,13,"#EXT-X-START:")==0)continue;
            if(line.compare(0,7,"#EXT-X-")==0&&
               line.compare(0,15,"#EXT-X-VERSION:")!=0&&
               line.compare(0,22,"#EXT-X-TARGETDURATION:")!=0&&
               line.compare(0,22,"#EXT-X-MEDIA-SEQUENCE:")!=0&&
               line.compare(0,21,"#EXT-X-PLAYLIST-TYPE:")!=0&&
               line!="#EXT-X-INDEPENDENT-SEGMENTS")return false;
        }else{
            std::string url;if(!Resolve(base,line,url))return false;
            if(variant.bandwidth){
                if(result.variants.size()>=32||!Extension(url,".m3u8"))return false;
                variant.url=url;result.variants.push_back(variant);variant=Variant{};
            }else if(duration){
                if(result.segments.size()>=MaxSegments||(result.init.empty()?!Extension(url,".ts"):(!Extension(url,".m4s")&&!Extension(url,".mp4"))))return false;
                result.segments.push_back(url);duration=false;
            }else return false;
        }
    }
    return !variant.bandwidth&&!duration&&((!result.variants.empty()&&result.segments.empty())||
        ((result.ended||(allowLive&&result.hasSequence&&result.target))&&!result.segments.empty()&&result.variants.empty()));
}
inline std::string Choose(const List& list,unsigned cap){
    const Variant* best=nullptr;
    for(const auto& v:list.variants)if(v.compatible&&v.bandwidth<=cap&&(!best||v.bandwidth>best->bandwidth))best=&v;
    return best?best->url:"";
}
// Sequential VOD or opt-in live TS/fMP4. A sliding window must preserve sequence
// identity. Never replay a segment after any bytes were delivered.
enum class Failure { None, Transport, WindowExpired, StalePlaylist, InvalidPlaylist, InvalidSegment, Cancelled };
inline const char* FailureLabel(Failure f){
    switch(f){
        case Failure::Transport:return "Falha de transporte HLS.";
        case Failure::WindowExpired:return "Janela HLS avancou alem do segmento esperado.";
        case Failure::StalePlaylist:return "Playlist ao vivo nao avancou.";
        case Failure::InvalidPlaylist:return "Playlist HLS invalida ou identidade alterada.";
        case Failure::InvalidSegment:return "Segmento HLS invalido ou incompleto.";
        case Failure::Cancelled:return "Reproducao cancelada.";
        default:return "Fonte HLS indisponivel.";
    }
}
inline bool RecoverLive(Failure f){return f==Failure::Transport||f==Failure::WindowExpired||f==Failure::StalePlaylist;}
template<class Reader> class VodReader {
public:
    using Open=std::function<std::unique_ptr<Reader>(const std::string&,bool)>;
    using Wait=std::function<bool(unsigned)>; // milliseconds; false cancels
    using Clock=std::function<uint64_t()>; // monotonic milliseconds
    bool EnableStartupAdaptation(Clock clock){if(started_||!clock)return false;clock_=std::move(clock);return true;}
    unsigned StartupThroughput()const{return throughput_;}
    explicit VodReader(Open open,Wait wait={}):open_(std::move(open)),wait_(std::move(wait)){}
    bool Start(const std::string& url,unsigned cap,bool allowLive=false){
        if(started_)return false;started_=true;allowLive_=allowLive;
        if(!Extension(url,".m3u8"))return false;
        playlist_=url;
        List list;if(!FetchInitial(playlist_,list))return false;
        if(!list.variants.empty()){
            auto candidates=list.variants;
            std::stable_sort(candidates.begin(),candidates.end(),[](const Variant& a,const Variant& b){return a.bandwidth>b.bandwidth;});
            bool selected=false;unsigned attempts=0;
            for(const auto& v:candidates){
                if(!v.compatible||v.bandwidth>cap)continue;
                if(++attempts>3)break;
                List media;
                if(!Fetch(v.url,media)||!media.variants.empty()||media.segments.empty()||
                    (!media.ended&&(!allowLive_||!wait_))||(!media.init.empty()&&!buffered_))continue;
                if(clock_&&buffered_&&media.init.empty()){
                    // Sample the first intended TS segment before publishing any bytes.
                    // Never use sequence numbers to align distinct renditions mid-playback.
                    const size_t index=!media.ended&&media.segments.size()>3?media.segments.size()-3:0;
                    const auto before=clock_();
                    const bool sampled=BufferSegment(media.segments[index]);
                    const auto after=clock_();
                    const bool oversized=streamingSegment_;streamingSegment_=false;
                    if(!sampled&&!oversized){segment_.clear();continue;}
                    if(sampled&&after>before&&after-before<=120000&&segment_.size()>=16384){
                        const uint64_t rate=uint64_t(segment_.size())*8000/(after-before);
                        throughput_=static_cast<unsigned>(std::min<uint64_t>(rate,100000000));
                        const unsigned safe=static_cast<unsigned>(uint64_t(throughput_)*75/100);
                        bool lower=false;
                        for(const auto& alt:candidates)
                            if(alt.compatible&&alt.bandwidth<v.bandwidth&&alt.bandwidth<=safe){lower=true;break;}
                        if(v.bandwidth>safe&&lower&&attempts<3){cap=std::min(cap,safe);segment_.clear();continue;}
                    }
                }
                playlist_=v.url;list=std::move(media);selectedBitrate_=v.bandwidth;selected=true;break;
            }
            if(!selected)return false;
        }
        if(list.segments.empty()||(!list.ended&&(!allowLive_||!wait_)))return false;
        // Fragmented MP4 requires atomic buffering and a fixed init map.
        if(!list.init.empty()&&!buffered_)return false;
        next_=list.sequence;
        if(!list.ended&&list.segments.size()>3)next_+=list.segments.size()-3;
        list_=std::move(list);valid_=true;return true;
    }
    bool Fragmented()const{return valid_&&!list_.init.empty();}
    bool Live()const{return valid_&&!list_.ended;}
    Failure LastFailure()const{return failure_;}
    unsigned SelectedBitrate()const{return selectedBitrate_;}
    // Optional atomic delivery for short TS segments; larger segments are rejected in this mode.
    bool EnableBufferedSegments(bool allowLargeStreaming=false){if(started_)return false;buffered_=true;largeStreaming_=allowLargeStreaming;return true;}
    unsigned RecoveredSegments()const{return recovered_;}
    int Read(unsigned char* buffer,unsigned capacity){
        if(failed_||!valid_||!buffer||!capacity)return -1;
        if(Fragmented()&&!initDone_){
            if(segment_.empty()&&!BufferSegment(list_.init,true))return Fail(segmentFailure_);
            const auto n=std::min<size_t>(capacity,segment_.size()-segmentOffset_);
            std::copy_n(segment_.data()+segmentOffset_,n,buffer);segmentOffset_+=n;
            if(segmentOffset_==segment_.size()){segment_.clear();segmentOffset_=0;initDone_=true;}
            return static_cast<int>(n);
        }
        unsigned refreshes=0,attempts=0;
        for(;;){
            const uint64_t end=list_.sequence+list_.segments.size();
            if(next_>=end){
                if(list_.ended)return 0;
                if(++refreshes>3)return Fail(refreshFailure_);
                if(!wait_||!wait_(std::min(5000u,std::max(500u,list_.target*500u))))return Fail(Failure::Cancelled);
                List updated;
                if(!Fetch(playlist_,updated)){
                    if(fetchFailure_==Failure::InvalidPlaylist)return Fail(fetchFailure_);
                    refreshFailure_=Failure::Transport;continue;
                }
                if(!Accept(updated))return Fail(refreshFailure_);
                refreshFailure_=Failure::StalePlaylist;
                continue;
            }
            if(next_<list_.sequence)return Fail(Failure::WindowExpired);
            if(buffered_&&!streamingSegment_){
                if(segment_.empty()&&!BufferSegment(list_.segments[static_cast<size_t>(next_-list_.sequence)])){
                    if(!streamingSegment_)return Fail(segmentFailure_);
                    // No bytes from this segment have reached the decoder. Reopen in streaming mode.
                    continue;
                }
                const auto n=std::min<size_t>(capacity,segment_.size()-segmentOffset_);
                std::copy_n(segment_.data()+segmentOffset_,n,buffer);segmentOffset_+=n;
                if(segmentOffset_==segment_.size()){segment_.clear();segmentOffset_=0;++next_;}
                return static_cast<int>(n);
            }
            if(!reader_){
                reader_=open_(list_.segments[static_cast<size_t>(next_-list_.sequence)],false);
                bytes_=0;
                if(!reader_){
                    if(++attempts<=2&&wait_){if(wait_(250u*attempts))continue;return Fail(Failure::Cancelled);}
                    return Fail(Failure::Transport);
                }
            }
            int n=reader_->Read(buffer,capacity);
            if(n<0){
                if(!bytes_&&++attempts<=2&&wait_){if(!wait_(250u*attempts))return Fail(Failure::Cancelled);reader_.reset();continue;}
                return Fail(Failure::Transport);
            }
            if(static_cast<unsigned>(n)>capacity||static_cast<size_t>(n)>MaxSegment-bytes_)return Fail(Failure::InvalidSegment);
            if(n){bytes_+=n;return n;}
            if(!bytes_||bytes_%188)return Fail(Failure::InvalidSegment);
            reader_.reset();++next_;attempts=0;streamingSegment_=false;
        }
    }
private:
    bool BufferSegment(const std::string& url,bool init=false){
        unsigned char block[8192];segmentFailure_=Failure::Transport;
        for(unsigned attempt=0;attempt<3;++attempt){
            if(attempt&&!wait_)break;
            if(attempt&&!wait_(250u*attempt)){segmentFailure_=Failure::Cancelled;segment_.clear();return false;}
            segment_.clear();segmentOffset_=0;
            if(segment_.capacity()<MaxBufferedSegment)segment_.reserve(MaxBufferedSegment);
            auto reader=open_(url,false);if(!reader){segmentFailure_=Failure::Transport;continue;}
            for(;;){
                const int n=reader->Read(block,sizeof block);
                if(n<0){segmentFailure_=Failure::Transport;break;}
                if(n==0){
                    if(!segment_.empty()&&(Fragmented()?ValidBoxes(segment_,init):segment_.size()%188==0)){
                        if(attempt)++recovered_;return true;
                    }
                    segmentFailure_=Failure::InvalidSegment;break;
                }
                if(n>static_cast<int>(sizeof block)){segmentFailure_=Failure::InvalidSegment;segment_.clear();return false;}
                if(static_cast<size_t>(n)>MaxBufferedSegment-segment_.size()){
                    segmentFailure_=Failure::InvalidSegment;std::vector<unsigned char>().swap(segment_);
                    if(largeStreaming_&&!Fragmented())streamingSegment_=true;
                    return false;
                }
                segment_.insert(segment_.end(),block,block+n);
            }
        }
        segment_.clear();return false;
    }
    static bool ValidBoxes(const std::vector<unsigned char>& data,bool init){
        size_t off=0;bool main=false,media=false;
        while(off<data.size()){
            if(data.size()-off<8)return false;
            const auto* p=data.data()+off;
            uint32_t size=(uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];
            if(size<8||size>data.size()-off)return false;
            const std::string type(reinterpret_cast<const char*>(p+4),4);
            if(init){
                if(type=="moov")main=true;
                else if(type!="ftyp"&&type!="free"&&type!="skip")return false;
            }else{
                if(type=="moof")main=true;
                else if(type=="mdat"){if(!main)return false;media=true;}
                else if(type!="styp"&&type!="sidx"&&type!="free"&&type!="emsg"&&type!="prft")return false;
            }
            off+=size;
        }
        return main&&(init||media);
    }
    bool Accept(List& updated){
        refreshFailure_=Failure::InvalidPlaylist;
        if(updated.init!=list_.init||!updated.variants.empty()||updated.segments.empty()||!updated.hasSequence||
           updated.sequence<list_.sequence||updated.sequence+updated.segments.size()<next_)return false;
        if(updated.sequence>next_){refreshFailure_=Failure::WindowExpired;return false;}
        const uint64_t overlapEnd=std::min(list_.sequence+list_.segments.size(),updated.sequence+updated.segments.size());
        for(uint64_t seq=updated.sequence;seq<overlapEnd;++seq)
            if(list_.segments[static_cast<size_t>(seq-list_.sequence)]!=updated.segments[static_cast<size_t>(seq-updated.sequence)])return false;
        list_=std::move(updated);return true;
    }
    bool FetchInitial(const std::string& url,List& list){
        for(unsigned attempt=0;attempt<3;++attempt){
            if(attempt&&(!wait_||!wait_(250u*attempt)))return false;
            if(Fetch(url,list))return true;
            if(fetchFailure_==Failure::InvalidPlaylist)return false;
        }
        return false;
    }
    bool Fetch(const std::string& url,List& list){
        fetchFailure_=Failure::Transport;auto reader=open_(url,true);if(!reader)return false;
        std::string text;unsigned char block[4096];
        for(;;){int n=reader->Read(block,sizeof block);if(n<0)return false;if(n>int(sizeof block)||size_t(n)>MaxPlaylist-text.size()){fetchFailure_=Failure::InvalidPlaylist;return false;}if(!n)break;text.append(reinterpret_cast<char*>(block),n);}
        if(!Parse(url,text,list,allowLive_)){fetchFailure_=Failure::InvalidPlaylist;return false;}return true;
    }
    int Fail(Failure reason){failed_=true;failure_=reason;reader_.reset();return -1;}
    Failure failure_=Failure::None,refreshFailure_=Failure::StalePlaylist,segmentFailure_=Failure::Transport,fetchFailure_=Failure::Transport;
    Open open_;Wait wait_;Clock clock_;unsigned throughput_=0;std::unique_ptr<Reader> reader_;
    std::string playlist_;List list_;
    uint64_t next_=0;size_t bytes_=0;unsigned selectedBitrate_=0;
    bool started_=false,failed_=false,valid_=false,allowLive_=false,buffered_=false;
    std::vector<unsigned char> segment_;size_t segmentOffset_=0;unsigned recovered_=0;
    bool largeStreaming_=false,streamingSegment_=false,initDone_=false;
};
}
