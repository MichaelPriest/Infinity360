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
    std::puts("live sequence, retry, overlap, stale window and partial failure passed");
    return 0;
}
