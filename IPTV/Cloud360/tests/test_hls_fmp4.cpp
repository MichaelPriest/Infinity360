#include "hls_vod.h"
#include "container_decode.h"
#include <fstream>
#include <cstring>
struct Reader{
    std::string data;size_t pos=0;
    int Read(unsigned char* b,unsigned cap){
        const auto n=std::min<size_t>(cap,data.size()-pos);
        if(n)std::memcpy(b,data.data()+pos,n);pos+=n;return static_cast<int>(n);
    }
};
int main(int argc,char** argv){
    if(argc!=2)return 2;
    using namespace CloudHls;
    const std::string base="https://example.invalid/index.m3u8";
    List l;
    if(Parse(base,"#EXTM3U\n#EXT-X-MAP:URI=\"https://other.invalid/init.mp4\"\n#EXTINF:1,\na.m4s\n#EXT-X-ENDLIST\n",l))return 1;
    auto open=[&](const std::string& url,bool)->std::unique_ptr<Reader>{
        const auto name=url.substr(url.find_last_of('/')+1);
        std::ifstream in(std::string(argv[1])+name,std::ios::binary);
        if(!in)return {};
        return std::make_unique<Reader>(Reader{std::string(std::istreambuf_iterator<char>(in),{})});
    };
    VodReader<Reader> r(open);
    if(!r.EnableBufferedSegments()||!r.Start(base,2000000)||!r.Fragmented())return 1;
    auto* s=cloud_container_open_mp4([](void* u,uint8_t* b,unsigned n){return static_cast<VodReader<Reader>*>(u)->Read(b,n);},&r,nullptr,nullptr);
    if(!s)return 1;
    int rc;do{rc=cloud_container_step(s,8);}while(rc>0);
    const auto report=*cloud_container_report(s);cloud_container_close(s);
    return rc||report.video_frames<50||report.audio_samples<400000?1:0;
}
