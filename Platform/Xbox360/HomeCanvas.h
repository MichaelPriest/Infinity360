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
    void Text(int x, int y, const std::string& text, int size, unsigned color, int maxWidth);
private:
    std::vector<unsigned> frame_, art_;
};
