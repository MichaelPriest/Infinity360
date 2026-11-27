#include "container_decode.h"
#include <cstdio>
#include <algorithm>
struct Source{
    FILE* file=nullptr;long length=0;unsigned seeks=0;bool reject=false;
    static int Read(void* u,uint8_t* b,unsigned n){
        auto& s=*static_cast<Source*>(u);
        const auto got=fread(b,1,std::min(n,997u),s.file);
        return got?static_cast<int>(got):ferror(s.file)?-1:0;
    }
    static int64_t Seek(void* u,int64_t p){
        auto& s=*static_cast<Source*>(u);++s.seeks;
        if(s.reject||p<0||p>s.length||fseek(s.file,static_cast<long>(p),SEEK_SET))return -1;
        return p;
    }
};
int main(int argc,char** argv){
    if(argc!=2)return 2;
    Source r;r.file=fopen(argv[1],"rb");if(!r.file)return 1;
    if(fseek(r.file,0,SEEK_END)||(r.length=ftell(r.file))<=0){fclose(r.file);return 1;}
    auto* s=cloud_container_open_mp4_seekable(Source::Read,Source::Seek,r.length,&r,nullptr,nullptr);
    if(!s){fclose(r.file);return 1;}
    const auto duration=cloud_container_duration(s);
    bool ok=duration>8000000&&r.seeks>1;
    int rc;do{rc=cloud_container_step(s,16);}while(rc>0);
    const auto full=*cloud_container_report(s);
    ok=ok&&rc==0&&full.video_frames>0&&full.audio_samples>0;
    ok=ok&&cloud_container_seek(s,6000000)==0;
    do{rc=cloud_container_step(s,16);}while(rc>0);
    const auto tail=*cloud_container_report(s);
    ok=ok&&rc==0&&tail.video_frames>0&&tail.video_frames<full.video_frames;
    ok=ok&&cloud_container_seek(s,0)==0;
    do{rc=cloud_container_step(s,16);}while(rc>0);
    ok=ok&&rc==0&&cloud_container_report(s)->video_frames==full.video_frames;
    cloud_container_close(s);
    r.reject=true;
    s=cloud_container_open_mp4_seekable(Source::Read,Source::Seek,r.length,&r,nullptr,nullptr);
    if(s){ok=false;cloud_container_close(s);}
    fclose(r.file);return ok?0:1;
}
