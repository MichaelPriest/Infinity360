#include "hls_vod.h"
#include <cstring>
#include <map>
#include <cstdio>
struct Reader {
    std::string data;size_t pos=0;bool failure=false;
    int Read(unsigned char* out,unsigned cap){
        if(failure&&pos>=data.size())return -1;
        const auto n=std::min<size_t>(cap,data.size()-pos);
        if(n)std::memcpy(out,data.data()+pos,n);pos+=n;return static_cast<int>(n);
    }
};
using namespace CloudHls;
static std::string Playlist(unsigned seq,const std::vector<std::string>& segments,bool end=false){
    std::string p="#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXT-X-MEDIA-SEQUENCE:"+std::to_string(seq)+"\n";
    for(const auto& s:segments)p+="#EXTINF:2,\n"+s+".ts\n";
    if(end)p+="#EXT-X-ENDLIST\n";return p;
}
int main(){
    const std::string base="https://example.invalid/live/index.m3u8";
    const auto first=Playlist(10,{"a","b","c"}),second=Playlist(11,{"b","c","d"}),third=Playlist(12,{"c","d","e"},true);
    List l;if(Parse(base,first,l)||!Parse(base,first,l,true)||l.sequence!=10||l.target!=2)return 1;
    unsigned requests=0,waits=0,attempts=0;
    std::map<std::string,unsigned> hits;
    auto open=[&](const std::string& url,bool playlist)->std::unique_ptr<Reader>{
        if(playlist){++requests;return std::make_unique<Reader>(Reader{requests==1?first:requests==2?second:third});}
        auto name=url.substr(url.find_last_of('/')+1);++hits[name];
        if(name=="d.ts"&&++attempts==1)return {};
        return std::make_unique<Reader>(Reader{std::string(188,name[0])});
    };
    VodReader<Reader> reader(open,[&](unsigned ms){++waits;return ms>=250&&ms<=5000;});
    if(!reader.Start(base,2000000,true))return 1;
    std::string bytes;unsigned char block[97];
    for(;;){int n=reader.Read(block,sizeof block);if(n<0)return 1;if(!n)break;bytes.append(reinterpret_cast<char*>(block),n);}
    std::string expected;for(char c:std::string("abcde"))expected+=std::string(188,c);
    if(bytes!=expected||requests!=3||attempts!=2||waits!=3||hits["a.ts"]!=1||hits["b.ts"]!=1||hits["c.ts"]!=1)return 1;
    auto failure=[&](const std::string& refresh,bool partial){
        unsigned count=0;
        VodReader<Reader> bad([&](const std::string&,bool playlist)->std::unique_ptr<Reader>{
            if(playlist)return std::make_unique<Reader>(Reader{++count==1?first:refresh});
            return std::make_unique<Reader>(Reader{std::string(partial?97:188,'x'),0,partial});
        },[](unsigned){return true;});
        if(!bad.Start(base,2000000,true))return false;
        for(unsigned i=0;i<100;++i){int n=bad.Read(block,sizeof block);if(n<0)return bad.Read(block,sizeof block)<0;if(!n)return false;}
        return false;
    };
    if(!failure(Playlist(15,{"f","g"}),false)||!failure(Playlist(11,{"changed","c","d"}),false)||
       !failure(first,false)||!failure(second,true))return 1;
    // Retry startup only before media is delivered; cancellation is terminal.
    unsigned startup=0,delays=0;
    VodReader<Reader> retry([&](const std::string&,bool playlist)->std::unique_ptr<Reader>{
        if(playlist){if(++startup<3)return {};return std::make_unique<Reader>(Reader{first});}
        return std::make_unique<Reader>(Reader{std::string(188,'x')});
    },[&](unsigned ms){++delays;return ms==250||ms==500;});
    if(!retry.Start(base,2000000,true)||startup!=3||delays!=2||retry.Read(block,sizeof block)!=97)return 1;
    unsigned cancelled=0;
    VodReader<Reader> stop([&](const std::string&,bool)->std::unique_ptr<Reader>{++cancelled;return {};},[](unsigned){return false;});
    if(stop.Start(base,2000000,true)||cancelled!=1)return 1;
    const auto dated="#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXT-X-MEDIA-SEQUENCE:10\n#EXT-X-PROGRAM-DATE-TIME:2026-11-26T20:00:00Z\n#EXTINF:2,\na.ts\n";
    if(!Parse(base,dated,l,true)||l.segments.size()!=1)return 1;
    unsigned segmentAttempts=0;
    VodReader<Reader> buffered([&](const std::string&,bool playlist)->std::unique_ptr<Reader>{
        if(playlist)return std::make_unique<Reader>(Reader{Playlist(1,{"a"},true)});
        if(++segmentAttempts==1)return std::make_unique<Reader>(Reader{std::string(97,'x'),0,true});
        return std::make_unique<Reader>(Reader{std::string(188,'y')});
    },[](unsigned){return true;});
    if(!buffered.EnableBufferedSegments()||!buffered.Start(base,2000000,true))return 1;
    std::string recovered;
    for(;;){const int n=buffered.Read(block,sizeof block);if(n<0)return 1;if(!n)break;recovered.append(reinterpret_cast<char*>(block),n);}
    if(recovered!=std::string(188,'y')||buffered.RecoveredSegments()!=1||segmentAttempts!=2)return 1;
    VodReader<Reader> oversized([&](const std::string&,bool playlist)->std::unique_ptr<Reader>{
        return std::make_unique<Reader>(Reader{playlist?Playlist(1,{"a"},true):std::string(MaxBufferedSegment+188,'z')});
    },[](unsigned){return true;});
    if(!oversized.EnableBufferedSegments()||!oversized.Start(base,2000000,true)||oversized.Read(block,sizeof block)>=0)return 1;
    // Large segment fallback must reopen before exposing any buffered bytes.
    unsigned largeOpens=0;
    const size_t largeSize=(MaxBufferedSegment/188+1)*188;
    VodReader<Reader> large([&](const std::string&,bool playlist)->std::unique_ptr<Reader>{
        if(playlist)return std::make_unique<Reader>(Reader{Playlist(1,{"large","next"},true)});
        ++largeOpens;
        return std::make_unique<Reader>(Reader{largeOpens==1?std::string(largeSize,'a'):largeOpens==2?std::string(largeSize,'b'):std::string(188,'c')});
    },[](unsigned){return true;});
    if(!large.EnableBufferedSegments(true)||!large.Start(base,2000000,true))return 1;
    size_t total=0;unsigned char bulk[8192];
    for(;;){const int n=large.Read(bulk,sizeof bulk);if(n<0)return 1;if(!n)break;
        for(int i=0;i<n;++i)if(bulk[i]!=(total+size_t(i)<largeSize?'b':'c'))return 1;
        total+=size_t(n);}
    if(total!=largeSize+188||largeOpens!=3)return 1;
    // fMP4 sliding window: initialization emitted once; map changes fail.
    const auto mp4List=[](unsigned seq,const std::vector<std::string>& names,bool ended,const std::string& map){
        std::string p="#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXT-X-MEDIA-SEQUENCE:"+std::to_string(seq)+"\n#EXT-X-MAP:URI=\""+map+"\"\n";
        for(const auto& n:names)p+="#EXTINF:2,\n"+n+".m4s\n";
        if(ended)p+="#EXT-X-ENDLIST\n";return p;
    };
    const auto box=[](const std::string& type,char payload){
        std::string b(4,'\0');b[3]=9;return b+type+payload;
    };
    const auto init=box("ftyp",'i')+box("moov",'i');
    const auto frag=[&](char ch){return box("moof",ch)+box("mdat",ch);};
    auto liveMp4=[&](bool changed){
        unsigned manifests=0,inits=0;
        VodReader<Reader> v([&](const std::string& url,bool playlist)->std::unique_ptr<Reader>{
            if(playlist)return std::make_unique<Reader>(Reader{++manifests==1?mp4List(1,{"a","b"},false,"init.mp4"):mp4List(2,{"b","c"},true,changed?"other.mp4":"init.mp4")});
            auto name=url.substr(url.find_last_of('/')+1);
            if(name=="init.mp4"){++inits;return std::make_unique<Reader>(Reader{init});}
            return std::make_unique<Reader>(Reader{frag(name[0])});
        },[](unsigned){return true;});
        if(!v.EnableBufferedSegments()||!v.Start(base,2000000,true)||!v.Fragmented())return false;
        std::string delivered;
        for(unsigned i=0;i<30;++i){
            const int n=v.Read(block,sizeof block);
            if(n<0)return changed&&inits==1&&delivered==init+frag('a')+frag('b');
            if(!n)return !changed&&inits==1&&delivered==init+frag('a')+frag('b')+frag('c');
            delivered.append(reinterpret_cast<char*>(block),n);
        }
        return false;
    };
    if(!liveMp4(false)||!liveMp4(true))return 1;
    // Classify a live window skip separately from identity changes; only the former permits a fresh decoder restart.
    auto reason=[&](const std::string& refresh){
        unsigned count=0;
        VodReader<Reader> r([&](const std::string&,bool playlist)->std::unique_ptr<Reader>{
            return std::make_unique<Reader>(Reader{playlist?(++count==1?first:refresh):std::string(188,'x')});
        },[](unsigned){return true;});
        if(!r.Start(base,2000000,true))return Failure::None;
        for(unsigned i=0;i<100;++i){int n=r.Read(block,sizeof block);if(n<0)return r.LastFailure();if(!n)break;}
        return Failure::None;
    };
    if(reason(Playlist(15,{"f","g"}))!=Failure::WindowExpired||
       reason(Playlist(11,{"changed","c","d"}))!=Failure::InvalidPlaylist||
       reason(first)!=Failure::StalePlaylist||
       !RecoverLive(Failure::Transport)||!RecoverLive(Failure::WindowExpired)||!RecoverLive(Failure::StalePlaylist)||
       RecoverLive(Failure::InvalidPlaylist)||RecoverLive(Failure::InvalidSegment)||RecoverLive(Failure::Cancelled))return 1;
    // Cooperative prefetch opens at most one known segment and reuses its reader.
    std::map<std::string,unsigned> prefHits;unsigned polls=0;
    VodReader<Reader> pref([&](const std::string& url,bool playlist)->std::unique_ptr<Reader>{
        if(playlist)return std::make_unique<Reader>(Reader{Playlist(1,{"a","b","c"},true)});
        const auto name=url.substr(url.find_last_of('/')+1);++prefHits[name];
        return std::make_unique<Reader>(Reader{std::string(188,name[0])});
    },[](unsigned){return true;});
    if(!pref.EnableBufferedSegments()||!pref.EnablePrefetch([&](Reader&){++polls;return true;})||!pref.Start(base,2000000))return 1;
    pref.Prefetch();pref.Prefetch();
    if(prefHits["a.ts"]!=1||prefHits["b.ts"]||polls!=2)return 1;
    std::string output;
    for(unsigned i=0;i<20;++i){
        const int n=pref.Read(block,sizeof block);if(n<0)return 1;if(!n)break;
        output.append(reinterpret_cast<char*>(block),n);pref.Prefetch();
    }
    if(output!=std::string(188,'a')+std::string(188,'b')+std::string(188,'c')||
       prefHits["a.ts"]!=1||prefHits["b.ts"]!=1||prefHits["c.ts"]!=1||pref.PrefetchedSegments()!=3)return 1;
    // Failed speculative transfer falls back to the normal atomic read, without duplicate bytes.
    unsigned opened=0;
    VodReader<Reader> fallback([&](const std::string&,bool playlist)->std::unique_ptr<Reader>{
        if(playlist)return std::make_unique<Reader>(Reader{Playlist(1,{"a"},true)});
        ++opened;return std::make_unique<Reader>(Reader{std::string(188,'x')});
    });
    if(!fallback.EnableBufferedSegments()||!fallback.EnablePrefetch([](Reader&){return false;})||!fallback.Start(base,2000000))return 1;
    fallback.Prefetch();fallback.Prefetch();if(opened!=1)return 1;
    if(fallback.Read(block,sizeof block)!=97||fallback.Read(block,sizeof block)!=91||fallback.Read(block,sizeof block)!=0||opened!=2||fallback.PrefetchedSegments())return 1;
    std::puts("live sequence, retry, overlap, stale window, cooperative prefetch and partial failure passed");
    return 0;
}
