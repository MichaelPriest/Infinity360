#include "HomeCanvas.h"
#include "GlyphWidths.h"
#include <algorithm>
#include <cstdio>

namespace {
const int ArtWidth = 1024, ArtHeight = 512;
unsigned pixel(const unsigned char* b) {
    return (unsigned(b[3])<<24) | (unsigned(b[2])<<16) | (unsigned(b[1])<<8) | b[0];
}
unsigned blend(unsigned fg, unsigned bg, unsigned a) {
    if (a == 255) return 0xff000000u | fg;
    unsigned rb = (((fg & 0xff00ff)*a + (bg & 0xff00ff)*(256-a)) >> 8) & 0xff00ff;
    unsigned g = (((fg & 0xff00)*a + (bg & 0xff00)*(256-a)) >> 8) & 0xff00;
    return 0xff000000u | rb | g;
}
// Local file names may be ANSI. M3U names are usually UTF-8.
unsigned nextGlyph(const std::string& text, size_t& i) {
    unsigned first = static_cast<unsigned char>(text[i++]);
    if (first < 0xc2 || first > 0xf4) return first;
    size_t extra = first < 0xe0 ? 1 : first < 0xf0 ? 2 : 3;
    if (extra > text.size()-i) return first;
    unsigned cp = first & (0x3f >> extra);
    for (size_t n = 0; n < extra; ++n) {
        unsigned b = static_cast<unsigned char>(text[i+n]);
        if ((b & 0xc0) != 0x80) return first;
        cp = (cp<<6) | (b&63);
    }
    i += extra;
    switch(cp) {
        case 0x2018: case 0x2019: return 39;
        case 0x201c: case 0x201d: return '"';
        case 0x2013: case 0x2014: return '-';
        default:return cp<=255 ? cp : '?';
    }
}
}
HomeCanvas::HomeCanvas() : frame_(Width*Height) {}
bool HomeCanvas::LoadArt(const char* path) {
    FILE* f = fopen(path,"rb");
    if (!f) return false;
    unsigned char h[18];
    bool ok = fread(h,1,sizeof h,f)==sizeof h && !h[1] && h[2]==10 &&
        (h[12]+256*h[13])==ArtWidth && (h[14]+256*h[15])==ArtHeight &&
        h[16]==32 && !(h[17]&0xd0) && fseek(f,h[0],SEEK_CUR)==0;
    if (ok) art_.resize(ArtWidth*ArtHeight);
    size_t index=0;
    while (ok && index<art_.size()) {
        const int packet=fgetc(f);
        const size_t count=(packet & 127)+1;
        if (packet==EOF || count>art_.size()-index) { ok=false; break; }
        unsigned char bgra[4];
        for (size_t n=0; n<count; ++n) {
            if ((n==0 || !(packet&128)) && fread(bgra,1,4,f)!=4) { ok=false; break; }
            size_t row=index/ArtWidth;
            if (!(h[17]&32)) row=ArtHeight-1-row;
            art_[row*ArtWidth+index%ArtWidth]=pixel(bgra);
            ++index;
        }
    }
    fclose(f);
    if (!ok) art_.clear();
    return ok;
}
unsigned HomeCanvas::ThemeColor(unsigned color) const {
    if(!theme_)return color;
    const unsigned r=(color>>16)&255,g=(color>>8)&255,b=color&255;
    // Recolor only green UI accents and green-tinted panels; retain warning colors and artwork.
    if(g<=r||g<=b)return color;
    if(theme_==1)return (color&0xff000000u)|(r<<16)|(b<<8)|g;
    return (color&0xff000000u)|(g<<16)|(r<<8)|b;
}
void HomeCanvas::VideoRgb(const unsigned char* rgb,unsigned width,unsigned height,double pixelAspect){
    if(!rgb||!width||!height||width>1280||height>720)return;
    std::fill(frame_.begin(),frame_.end(),0xff000000u);
    if(!(pixelAspect>=0.25&&pixelAspect<=4.0))pixelAspect=1.0;
    const double aspect=width*pixelAspect/height;
    unsigned w=Width,h=Height;
    if(aspect>double(Width)/Height)h=std::max(1u,static_cast<unsigned>(Width/aspect));
    else w=std::max(1u,static_cast<unsigned>(Height*aspect));
    const unsigned x0=(Width-w)/2,y0=(Height-h)/2;
    for(unsigned y=0;y<h;++y)for(unsigned x=0;x<w;++x){
        const auto* p=rgb+3*((y*height/h)*width+x*width/w);
        frame_[(y+y0)*Width+x+x0]=0xff000000u|(unsigned(p[0])<<16)|(unsigned(p[1])<<8)|p[2];
    }
}
void HomeCanvas::Box(int x,int y,int w,int h,unsigned color,int radius) {
    color=ThemeColor(color);
    for (int j=std::max(0,-y); j<h && y+j<Height; ++j) {
        int cut=0, edge=std::min(j,h-1-j);
        if (edge<radius) {
            int dy=radius-edge-1;
            while (cut<radius && (radius-cut-1)*(radius-cut-1)+dy*dy>radius*radius) ++cut;
        }
        int start=std::max(0,x+cut), end=std::min(int(Width),x+w-cut);
        if (start<end) std::fill(frame_.begin()+(y+j)*Width+start,frame_.begin()+(y+j)*Width+end,color);
    }
}
void HomeCanvas::Background() {
    for(int y=0; y<Height; ++y) {
        const unsigned amount = 52*(Height-y)/Height;
        unsigned* row=&frame_[y*Width];
        for(int x=0;x<Width;++x) {
            const unsigned glow = amount*x/Width*x/Width;
            row[x] = ThemeColor(0xff031310u + (glow<<8) + glow/3);
        }
    }
}
void HomeCanvas::Image(int sx,int sy,int sw,int sh,int x,int y,int w,int h,unsigned tint) {
    if (art_.empty() || sw<=0 || sh<=0 || w<=0 || h<=0) return;
    for (int j=std::max(0,-y); j<h && y+j<Height; ++j) {
        const unsigned* src=&art_[(sy+j*sh/h)*ArtWidth+sx];
        unsigned* dst=&frame_[(y+j)*Width];
        for (int i=std::max(0,-x); i<w && x+i<Width; ++i) {
            unsigned color=src[i*sw/w], alpha=color>>24;
            if (alpha) dst[x+i]=blend(tint==0xffffffffu ? color : tint,dst[x+i],alpha);
        }
    }
}
bool HomeCanvas::Logo(const std::string& file,int x,int y,int w,int h) {
    if(w<=0 || h<=0 || x<0 || y<0 || x+w>Width || y+h>Height) return false;
    FILE* f=std::fopen(file.c_str(),"rb");if(!f) return false;
    std::vector<unsigned char> rgba(128*72*4);
    const bool good=std::fread(&rgba[0],1,rgba.size(),f)==rgba.size() && std::fgetc(f)==EOF;
    std::fclose(f);if(!good) return false;
    for(int j=0;j<h;++j) for(int i=0;i<w;++i) {
        const unsigned char* p=&rgba[4*((j*72/h)*128+i*128/w)];
        unsigned& dest=frame_[(y+j)*Width+x+i];
        if(p[3]) dest=blend((unsigned(p[0])<<16)|(unsigned(p[1])<<8)|p[2],dest,p[3]);
    }
    return true;
}
bool HomeCanvas::LocalArt(const std::string& file,bool cover,int x,int y,int w,int h) {
    if(file.empty() || w<=0 || h<=0 || x<0 || y<0 || x>=Width || y>=Height ||
       w>Width-x || h>Height-y) return false;
    const int sw=cover?160:640, sh=cover?240:360;
    size_t slot=0;
    for(;slot<localArt_.size();++slot)
        if(localArt_[slot].file==file && localArt_[slot].cover==cover) break;
    if(slot==localArt_.size()) {
        // FIFO cap: a full home screen requires at most 12 entries.
        // Missing and malformed files are cached too, avoiding per-frame disk I/O.
        if(localArt_.size()>=16) {localArt_.erase(localArt_.begin());--slot;}
        localArt_.push_back(ArtEntry());
        ArtEntry& entry=localArt_.back();
        entry.file=file;entry.cover=cover;
        FILE* f=std::fopen(file.c_str(),"rb");
        if(f) {
            entry.rgba.resize(static_cast<size_t>(sw)*sh*4);
            const bool ok=std::fread(&entry.rgba[0],1,entry.rgba.size(),f)==entry.rgba.size()
                && std::fgetc(f)==EOF && !std::ferror(f);
            std::fclose(f);
            if(!ok) std::vector<unsigned char>().swap(entry.rgba);
        }
    }
    const std::vector<unsigned char>& data=localArt_[slot].rgba;
    if(data.empty())return false;
    for(int j=0;j<h;++j) for(int i=0;i<w;++i) {
        const unsigned char* p=&data[4*((j*sh/h)*sw+i*sw/w)];
        unsigned& dest=frame_[(y+j)*Width+x+i];
        if(p[3])dest=blend((unsigned(p[0])<<16)|(unsigned(p[1])<<8)|p[2],dest,p[3]);
    }
    return true;
}
void HomeCanvas::Text(int x,int y,const std::string& text,int size,unsigned color,int maxWidth) {
    color=ThemeColor(color);
    const int right=x+maxWidth;
    for (size_t i=0; i<text.size();) {
        unsigned c=nextGlyph(text,i);
        if(c<32) c=' ';
        int index=int(c)-32;
        int advance=(glyphWidths[index]*size+11)/22;
        if(x+advance>right) break;
        const int cell=30*size/22;
        Image(index%32*30,234+index/32*34,30,34,x,y,cell,34*size/22,color);
        if (size>=28) Image(index%32*30,234+index/32*34,30,34,x+1,y,cell,34*size/22,color);
        x+=advance;
    }
}
