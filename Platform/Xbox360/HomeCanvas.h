#pragma once
#include <string>
#include <vector>
#include "UiLanguage.h"

class HomeCanvas {
public:
    enum { Width = 1280, Height = 720 };
    HomeCanvas();
    bool LoadArt(const char* path);
    const unsigned* Pixels() const { return &frame_[0]; }
    void Background();
    enum { ThemeCount=7 };
    void SetTheme(unsigned theme){theme_=theme<ThemeCount?theme:0;}
    void SetLanguage(unsigned language){language_=language<UiLanguage::Count?language:0;}
    const char* Tr(const char* key)const{return UiLanguage::Translate(language_,key);}
    unsigned Language()const{return language_;}
    static const char* ThemeName(unsigned theme);
    void VideoRgb(const unsigned char* pixels,unsigned width,unsigned height,double pixelAspect=1.0);
    void Box(int x, int y, int w, int h, unsigned color, int radius = 0);
    void Image(int sx, int sy, int sw, int sh, int x, int y, int w, int h, unsigned tint = 0xffffffffu);
    bool Logo(const std::string& file, int x, int y, int w, int h);
    // Fixed-size local RGBA files, cached including failed reads until refresh.
    bool LocalArt(const std::string& file,bool cover,int x,int y,int w,int h);
    void ClearLocalArt() { localArt_.clear(); }
    void Text(int x, int y, const std::string& text, int size, unsigned color, int maxWidth);
private:
    struct ArtEntry {
        std::string file;
        bool cover;
        std::vector<unsigned char> rgba;
    };
    std::vector<ArtEntry> localArt_;
    std::vector<unsigned> frame_, art_;
    unsigned theme_=0,language_=0;
    unsigned ThemeColor(unsigned color) const;
};
