#pragma once
#include <string>

// Persist only enumerated settings; the wallpaper path is never user-supplied
// through the profile. Existing profile files default to the original look.
struct AppearanceSettings {
    enum Background { Gradient, Solid, Wallpaper, BackgroundCount };
    enum Artwork { Both, Covers, Backdrops, NoArtwork, ArtworkCount };
    unsigned background=Gradient, dim=1, artwork=Both;
    static constexpr unsigned DimCount=3;
    static unsigned Darkness(unsigned index){return index==0?60:index==2?90:75;}
    static const char* BackgroundName(unsigned i){return i==Solid?"Solido":i==Wallpaper?"Papel de parede":"Gradiente";}
    static const char* ArtworkName(unsigned i){return i==Covers?"Capas":i==Backdrops?"Fundos":i==NoArtwork?"Sem arte":"Capas e fundos";}
    bool ShowCover()const{return artwork==Both||artwork==Covers;}
    bool ShowBackdrop()const{return artwork==Both||artwork==Backdrops;}
    bool Valid()const{return background<BackgroundCount&&dim<DimCount&&artwork<ArtworkCount;}
    std::string Encode()const{return std::to_string(background)+":"+std::to_string(dim)+":"+std::to_string(artwork);}
    static bool Parse(const std::string& value,AppearanceSettings& out){
        if(value.size()!=5||value[1]!=':'||value[3]!=':'||
           value[0]<'0'||value[0]>'9'||value[2]<'0'||value[2]>'9'||value[4]<'0'||value[4]>'9')return false;
        AppearanceSettings a;a.background=unsigned(value[0]-'0');a.dim=unsigned(value[2]-'0');a.artwork=unsigned(value[4]-'0');
        if(!a.Valid())return false;out=a;return true;
    }
};
