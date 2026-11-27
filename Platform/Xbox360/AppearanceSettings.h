#pragma once
#include <string>

// Persist only enumerated settings; the wallpaper path is never user-supplied
// through the profile. Existing profile files default to the original look.
struct AppearanceSettings {
    enum Background { Gradient, Solid, Wallpaper, BackgroundCount };
    enum Artwork { Both, Covers, Backdrops, NoArtwork, ArtworkCount };
    enum GlassMode { LightGlass, BalancedGlass, DenseGlass, OpaqueGlass, GlassCount };
    unsigned background=Gradient, dim=1, artwork=Both, glass=BalancedGlass, animations=1;
    static const char* GlassName(unsigned i){return i==LightGlass?"Leve":i==DenseGlass?"Densa":i==OpaqueGlass?"Opaca":"Equilibrada";}
    unsigned Opacity(unsigned base)const{if(glass==OpaqueGlass)return 255;const unsigned scale=glass==LightGlass?42:glass==DenseGlass?84:65;return (base>255?255:base)*scale/100;}
    static constexpr unsigned DimCount=3;
    static unsigned Darkness(unsigned index){return index==0?60:index==2?90:75;}
    static const char* BackgroundName(unsigned i){return i==Solid?"Solido":i==Wallpaper?"Papel de parede":"Gradiente";}
    static const char* ArtworkName(unsigned i){return i==Covers?"Capas":i==Backdrops?"Fundos":i==NoArtwork?"Sem arte":"Capas e fundos";}
    bool ShowCover()const{return artwork==Both||artwork==Covers;}
    bool ShowBackdrop()const{return artwork==Both||artwork==Backdrops;}
    bool Valid()const{return background<BackgroundCount&&dim<DimCount&&artwork<ArtworkCount&&glass<GlassCount&&animations<2;}
    std::string Encode()const{const auto old=std::to_string(background)+":"+std::to_string(dim)+":"+std::to_string(artwork);return glass==BalancedGlass&&animations==1?old:old+":"+std::to_string(glass)+":"+std::to_string(animations);}
    static bool Parse(const std::string& value,AppearanceSettings& out){
        if((value.size()!=5&&value.size()!=9)||value[1]!=':'||value[3]!=':'||
           value[0]<'0'||value[0]>'9'||value[2]<'0'||value[2]>'9'||value[4]<'0'||value[4]>'9')return false;
        AppearanceSettings a;a.background=unsigned(value[0]-'0');a.dim=unsigned(value[2]-'0');a.artwork=unsigned(value[4]-'0');
        if(value.size()==9){if(value[5]!=':'||value[7]!=':'||value[6]<'0'||value[6]>'9'||value[8]<'0'||value[8]>'9')return false;a.glass=unsigned(value[6]-'0');a.animations=unsigned(value[8]-'0');}
        if(!a.Valid())return false;out=a;return true;
    }
};
