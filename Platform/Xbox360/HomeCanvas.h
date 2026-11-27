#pragma once
#include <string>
#include <vector>
#include "UiLanguage.h"
#include "AppearanceSettings.h"

class HomeCanvas {
public:
    enum { Width = 1280, Height = 720 };
    HomeCanvas();
    bool LoadArt(const char* path);
    const unsigned* Pixels() const { return &frame_[0]; }
    void Background();
    void SetAppearance(const AppearanceSettings& appearance,const std::string& wallpaper);
    void ClearWallpaper(){wallpaperPath_.clear();wallpaperTried_=false;wallpaper_.clear();}
    bool WallpaperReady()const{return !wallpaper_.empty();}
    enum { ThemeCount=7 };
    void SetTheme(unsigned theme){theme_=theme<ThemeCount?theme:0;}
    void SetLanguage(unsigned language){language_=language<UiLanguage::Count?language:0;}
    const char* Tr(const char* key)const{return UiLanguage::Translate(language_,key);}
    unsigned Language()const{return language_;}
    static const char* ThemeName(unsigned theme);
    void VideoRgb(const unsigned char* pixels,unsigned width,unsigned height,double pixelAspect=1.0);
    void Box(int x, int y, int w, int h, unsigned color, int radius = 0);
    void Glass(int x,int y,int w,int h,bool focused=false);
    void Stroke(int x0,int y0,int x1,int y1,unsigned color,int width=3);
    void NeonBackground();
    void SeriesBackground();
    void SeriesFocus(int x,int y,int w,int h);
    void DimBackdrop();
    void SeriesTile(int x,int y,int w,int h,bool focused=false,unsigned opacity=204);
    void SetFocusAnimation(unsigned percent){focusAnimation_=percent>100?100:percent;}
    void TextFit(int x,int y,const std::string& text,int size,unsigned color,int maxWidth);
    void Translucent(int x,int y,int w,int h,unsigned color,unsigned opacity,int radius=0);
    void Shade(int x,int y,int w,int h,unsigned color,unsigned leftAlpha,unsigned rightAlpha);
    void CenterText(int x,int y,int w,const std::string& text,int size,unsigned color);
    void Image(int sx, int sy, int sw, int sh, int x, int y, int w, int h, unsigned tint = 0xffffffffu);
    bool ProfilePicture(const std::vector<unsigned char>& rgba,int x,int y,int size);
    bool AvatarFrame(const std::vector<unsigned char>& rgba,unsigned sw,unsigned sh,int x,int y,int w,int h);
    void BrandLogo(int x,int y);
    void ControllerButton(int kind,int x,int y,int height);
    void RasterIcon(int kind,int x,int y,int size,unsigned tint);
    bool Logo(const std::string& file, int x, int y, int w, int h);
    // Fixed-size local RGBA files, cached including failed reads until refresh.
    bool LocalArt(const std::string& file,bool cover,int x,int y,int w,int h,bool crop=false,int radius=0);
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
    unsigned theme_=0,language_=0,focusAnimation_=100;
    AppearanceSettings appearance_;
    std::string wallpaperPath_;
    bool wallpaperTried_=false;
    std::vector<unsigned char> wallpaper_;
    unsigned ThemeColor(unsigned color) const;
};
