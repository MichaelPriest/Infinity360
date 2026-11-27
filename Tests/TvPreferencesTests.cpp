#include "TvPreferences.h"
#include <cstdio>
int main(){
    using namespace TvPreferences;
    const std::string root="tv-preferences-test-";
    const std::string audio=root+"cloud360-audio.txt",cap=root+"infinity360-iptv-bitrate.txt",hd=root+"enable-iptv-1080p.txt";
    auto clean=[&](){for(const auto& f:{audio,cap,hd})for(const auto* suffix:{"",".tmp",".bak"})std::remove((f+suffix).c_str());};
    clean();
    bool ok=Cap(root)==DefaultCap&&LoadAudio(root).volume==100&&!FullHd(root);
    ok=ok&&SaveAudio(root,{30,true})&&LoadAudio(root).volume==30&&LoadAudio(root).muted;
    ok=ok&&!SaveAudio(root,{-1,false})&&LoadAudio(root).volume==30;
    ok=ok&&std::rename(audio.c_str(),(audio+".bak").c_str())==0&&LoadAudio(root).volume==30;
    ok=ok&&Write(audio,"101 0")&&LoadAudio(root).volume==100&&!LoadAudio(root).muted;
    ok=ok&&ParseCap("100000")==100000&&ParseCap("4000000")==4000000&&ParseCap("99999")==0&&ParseCap("4000001")==0&&ParseCap("2000000x")==0;
    ok=ok&&Write(cap,"800000")&&Cap(root)==800000&&std::rename(cap.c_str(),(cap+".bak").c_str())==0&&Cap(root)==800000;
    ok=ok&&Write(cap,"800000\n400000")&&Cap(root)==0;
    ok=ok&&Write(hd,"ENABLE-IPTV-1080P")&&FullHd(root)&&Write(hd,"DISABLED")&&!FullHd(root);
    clean();return ok?0:1;
}
