#include "hls_vod.h"
#include "container_decode.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <fstream>
struct Reader{
    std::string data;size_t pos=0;
    int Read(unsigned char* out,unsigned cap){size_t n=std::min<size_t>(std::min(cap,997u),data.size()-pos);std::memcpy(out,data.data()+pos,n);pos+=n;return static_cast<int>(n);}
};
int main(int argc,char** argv){
    if(argc!=2)return 2;
    using namespace CloudHls;
    const std::string base="https://example.invalid/live/master.m3u8";
    List list;
    if(!Parse(base,"#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=400000,CODECS=\"avc1,mp4a\"\nlow.m3u8\n#EXT-X-STREAM-INF:BANDWIDTH=2000000\nhigh.m3u8\n",list)||Choose(list,1000000)!="https://example.invalid/live/low.m3u8"||!Choose(list,100).empty())return 1;
    const std::string media="#EXTM3U\n#EXT-X-VERSION:3\n#EXT-X-TARGETDURATION:6\n#EXT-X-MEDIA-SEQUENCE:0\n#EXTINF:6.0,\na.ts\n#EXTINF:6.0,\nb.ts\n#EXT-X-ENDLIST\n";
    std::string out;
    if(Resolve(base,"https://other.invalid/a.ts",out)||Resolve(base,"../a.ts",out)||Resolve(base,"http://example.invalid/a.ts",out))return 1;
    for(const auto& bad:{"#EXTM3U\n#EXTINF:1,\na.ts\n","#EXTM3U\n#EXT-X-KEY:METHOD=AES-128\n#EXTINF:1,\na.ts\n#EXT-X-ENDLIST\n","#EXTM3U\n#EXT-X-MAP:URI=\"init.mp4\"\n#EXTINF:1,\na.ts\n#EXT-X-ENDLIST\n"})if(Parse(base,bad,list))return 1;
    std::ifstream input(argv[1],std::ios::binary);std::string ts((std::istreambuf_iterator<char>(input)),{});
    if(ts.size()<376||ts.size()%188)return 1;
    size_t split=(ts.size()/376)*188;
    std::map<std::string,std::string> files={{base,media},{"https://example.invalid/live/a.ts",ts.substr(0,split)},{"https://example.invalid/live/b.ts",ts.substr(split)}};
    auto open=[&](const std::string& url,bool){auto i=files.find(url);return i==files.end()?std::unique_ptr<Reader>{}:std::make_unique<Reader>(Reader{i->second});};
    VodReader<Reader> reader(open);if(!reader.Start(base,1000000))return 1;
    auto* s=cloud_container_open_ts([](void* p,uint8_t* b,unsigned n){return static_cast<VodReader<Reader>*>(p)->Read(b,n);},&reader,nullptr,nullptr);
    if(!s)return 1;
    int rc;do{rc=cloud_container_step(s,8);}while(rc>0);
    auto report=*cloud_container_report(s);cloud_container_close(s);
    auto* local=cloud_container_open(argv[1],nullptr,nullptr,1);if(!local)return 1;
    int lr;do{lr=cloud_container_step(local,8);}while(lr>0);
    auto expected=*cloud_container_report(local);cloud_container_close(local);
    return rc||lr||!report.video_frames||report.video_frames!=expected.video_frames||report.audio_samples!=expected.audio_samples?1:0;
}
