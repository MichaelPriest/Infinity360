#pragma once
#include "SavedFile.h"
#include <sstream>
#include <algorithm>

// Local preferences only; never persist a source URI or credentials.
namespace TvPreferences {
struct Audio {int volume=100;bool muted=false;};
inline bool Write(const std::string& file,const std::string& text){
    std::ofstream out((file+".tmp").c_str(),std::ios::binary);
    out<<text<<"\n";out.close();return bool(out)&&replaceSavedFile(file);
}
inline std::string Read(const std::string& file){
    std::ifstream in(savedFile(file).c_str(),std::ios::binary);
    char value[64]{};
    if(!in.getline(value,sizeof value)||in.peek()!=std::char_traits<char>::eof())return "";
    std::string result=value;if(!result.empty()&&result.back()=='\r')result.pop_back();return result;
}
inline Audio LoadAudio(const std::string& root){
    Audio audio;int volume=0,mute=0;char extra;
    std::istringstream in(Read(root+"cloud360-audio.txt"));
    if(in>>volume>>mute&&!(in>>extra)&&volume>=0&&volume<=100&&(mute==0||mute==1)){audio.volume=volume;audio.muted=mute!=0;}
    return audio;
}
inline bool SaveAudio(const std::string& root,const Audio& audio){
    if(audio.volume<0||audio.volume>100)return false;
    return Write(root+"cloud360-audio.txt",std::to_string(audio.volume)+" "+(audio.muted?"1":"0"));
}
inline unsigned Cap(const std::string& root){
    const auto value=Read(root+"infinity360-iptv-bitrate.txt");
    unsigned cap=0;for(char c:value){if(c<'0'||c>'9'||cap>400000)return 0;cap=cap*10+unsigned(c-'0');}
    return cap>=100000&&cap<=4000000?cap:0;
}
inline bool FullHd(const std::string& root){return Read(root+"enable-iptv-1080p.txt")=="ENABLE-IPTV-1080P";}
}
