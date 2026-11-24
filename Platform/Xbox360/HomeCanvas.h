#pragma once
#include <string>
#include <vector>

class HomeCanvas {
public:
    enum { Width = 1280, Height = 720 };
    HomeCanvas();
    bool LoadArt(const char* path);
    const unsigned* Pixels() const { return &frame_[0]; }
    void Background();
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
};
