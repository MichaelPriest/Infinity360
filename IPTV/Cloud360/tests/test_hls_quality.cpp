#include "hls_vod.h"
#include <cstring>
#include <map>
using namespace CloudHls;
struct Reader{
    std::string data;size_t pos=0;
    int Read(unsigned char* b,unsigned n){const auto count=std::min<size_t>(n,data.size()-pos);if(count)std::memcpy(b,data.data()+pos,count);pos+=count;return static_cast<int>(count);}
};
int main(){
    const std::string root="https://example.invalid/";
    const std::string master="#EXTM3U\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=3000000,RESOLUTION=1920x1080,CODECS=\"avc1.640028,mp4a.40.2\"\nfull.m3u8\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=2500000,CODECS=\"hvc1.1.6.L93.B0,mp4a.40.2\"\nhevc.m3u8\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=1900000,RESOLUTION=1280x720,CODECS=\"avc1.42001e,mp4a.40.2\"\nmissing.m3u8\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=1800000,AUDIO=\"external\"\nexternal.m3u8\n"
      "#EXT-X-STREAM-INF:BANDWIDTH=900000,RESOLUTION=640x360,CODECS=\"avc1.42001e,mp4a.40.2\"\nlow.m3u8\n";
    List l;if(!Parse(root+"master.m3u8",master,l)||l.variants.size()!=5||Choose(l,4000000)!=root+"missing.m3u8"||Choose(l,1000000)!=root+"low.m3u8")return 1;
    unsigned cap=DefaultBitrate;
    if(!BitrateCap("1000000",cap)||cap!=1000000||BitrateCap("99999",cap)||BitrateCap("4000001",cap)||BitrateCap("1.5",cap)||cap!=1000000)return 2;
    if(Parse(root+"master.m3u8","#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=123,RESOLUTION=640xBAD\na.m3u8\n",l))return 3;
    std::map<std::string,unsigned> hits;
    VodReader<Reader> reader([&](const std::string& url,bool playlist)->std::unique_ptr<Reader>{
        ++hits[url];
        if(url==root+"master.m3u8")return std::make_unique<Reader>(Reader{master});
        if(url==root+"low.m3u8"&&playlist)return std::make_unique<Reader>(Reader{"#EXTM3U\n#EXTINF:2,\nlow.ts\n#EXT-X-ENDLIST\n"});
        if(url==root+"low.ts"&&!playlist)return std::make_unique<Reader>(Reader{std::string(188,'x')});
        return {};
    });
    if(!reader.Start(root+"master.m3u8",DefaultBitrate)||reader.SelectedBitrate()!=900000||hits[root+"missing.m3u8"]!=1||hits[root+"external.m3u8"]||hits[root+"full.m3u8"]||hits[root+"hevc.m3u8"])return 4;
    unsigned char b[188];if(reader.Read(b,sizeof b)!=188||reader.Read(b,sizeof b)!=0)return 5;
    const std::string adaptiveMaster="#EXTM3U\n#EXT-X-STREAM-INF:BANDWIDTH=1800000\nhigh.m3u8\n#EXT-X-STREAM-INF:BANDWIDTH=400000\nlow.m3u8\n";
    uint64_t now=0;unsigned high=0,low=0;
    VodReader<Reader> adaptive([&](const std::string& url,bool playlist)->std::unique_ptr<Reader>{
        if(url==root+"master.m3u8")return std::make_unique<Reader>(Reader{adaptiveMaster});
        if(playlist)return std::make_unique<Reader>(Reader{"#EXTM3U\n#EXTINF:2,\n"+std::string(url==root+"high.m3u8"?"high":"low")+".ts\n#EXT-X-ENDLIST\n"});
        now+=1000;
        if(url==root+"high.ts"){++high;return std::make_unique<Reader>(Reader{std::string(188*400,'h')});}
        ++low;return std::make_unique<Reader>(Reader{std::string(188*400,'l')});
    });
    if(!adaptive.EnableBufferedSegments()||!adaptive.EnableStartupAdaptation([&](){return now;})||
       !adaptive.Start(root+"master.m3u8",DefaultBitrate)||adaptive.SelectedBitrate()!=400000||high!=1||low!=1)return 6;
    size_t total=0;
    for(;;){int n=adaptive.Read(b,sizeof b);if(n<0)return 7;if(!n)break;for(int i=0;i<n;++i)if(b[i]!='l')return 8;total+=n;}
    if(total!=188*400||low!=1||adaptive.StartupThroughput()!=601600)return 9;
    // Live quality reduction: a fast first segment, then three slow segments.
    // No cross-rendition bytes may be emitted before a fresh decoder is opened.
    uint64_t time=0;unsigned highReads=0,lowReads=0;
    const std::string live="#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXT-X-MEDIA-SEQUENCE:1\n#EXTINF:2,\na.ts\n#EXTINF:2,\nb.ts\n#EXTINF:2,\nc.ts\n";
    unsigned manifest=0;
    VodReader<Reader> running([&](const std::string& url,bool playlist)->std::unique_ptr<Reader>{
        if(url==root+"master.m3u8")return std::make_unique<Reader>(Reader{adaptiveMaster});
        if(url==root+"high.m3u8"){
            ++manifest;
            return std::make_unique<Reader>(Reader{manifest==1?live:"#EXTM3U\n#EXT-X-TARGETDURATION:2\n#EXT-X-MEDIA-SEQUENCE:2\n#EXTINF:2,\nb.ts\n#EXTINF:2,\nc.ts\n#EXTINF:2,\nd.ts\n"});
        }
        if(url==root+"low.m3u8"){++lowReads;return {};}
        if(!playlist){time+=(++highReads==1?100:1000);return std::make_unique<Reader>(Reader{std::string(188*400,'h')});}
        return {};
    },[](unsigned){return true;});
    if(!running.EnableBufferedSegments()||!running.EnableStartupAdaptation([&](){return time;})||
       !running.Start(root+"master.m3u8",DefaultBitrate,true)||running.SelectedBitrate()!=1800000)return 10;
    running.ArmRuntimeAdaptation();
    total=0;
    for(;;){int n=running.Read(b,sizeof b);if(n<0)break;if(!n)return 11;for(int i=0;i<n;++i)if(b[i]!='h')return 12;total+=n;if(total>188*400*4)return 13;}
    if(total!=188*400*4||running.LastFailure()!=Failure::LowerQuality||running.SuggestedBitrate()!=400000||lowReads||highReads!=4)return 14;
    return 0;
}
