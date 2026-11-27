#include "container_decode.h"
#include <cstdio>
#include <algorithm>
#include <string>
struct Reader{
    FILE* file=nullptr;
    unsigned long long bytes=0,limit=0;
    bool oversize=false;
    static int Read(void* user,uint8_t* buffer,unsigned cap){
        auto& s=*static_cast<Reader*>(user);
        if(s.oversize)return static_cast<int>(cap)+1;
        if(s.limit&&s.bytes>=s.limit)return -1;
        unsigned n=std::min(cap,997u);
        if(s.limit)n=static_cast<unsigned>(std::min<unsigned long long>(n,s.limit-s.bytes));
        const auto got=fread(buffer,1,n,s.file);s.bytes+=got;
        return got?static_cast<int>(got):ferror(s.file)?-1:0;
    }
};
int main(int argc,char** argv){
    if(argc!=2)return 2;
    const std::string path=argv[1];
    const bool mp4=path.size()>=4&&path.substr(path.size()-4)==".mp4";
    auto open=mp4?cloud_container_open_mp4:cloud_container_open_ts;
    CloudContainerReport expected{};
    auto* local=cloud_container_open(argv[1],nullptr,nullptr,1);
    if(!local)return 1;
    int rc;do{rc=cloud_container_step(local,16);}while(rc>0);
    expected=*cloud_container_report(local);cloud_container_close(local);if(rc)return 1;
    Reader r;r.file=fopen(argv[1],"rb");if(!r.file)return 1;
    auto* s=open(Reader::Read,&r,nullptr,nullptr);
    if(!s){fclose(r.file);return 1;}
    do{rc=cloud_container_step(s,3);}while(rc>0);
    const auto got=*cloud_container_report(s);
    bool ok=(!mp4||cloud_container_duration(s)==0)&&rc==0&&got.video_frames==expected.video_frames&&got.audio_samples==expected.audio_samples;
    cloud_container_close(s);
    const auto size=r.bytes;
    rewind(r.file);r.bytes=0;r.limit=size*3/4;
    s=open(Reader::Read,&r,nullptr,nullptr);
    if(s){do{rc=cloud_container_step(s,4);}while(rc>0);ok=ok&&rc<0;cloud_container_close(s);}
    rewind(r.file);r.bytes=0;r.oversize=true;
    s=open(Reader::Read,&r,nullptr,nullptr);
    if(s){ok=false;cloud_container_close(s);}
    fclose(r.file);
    return ok?0:1;
}
