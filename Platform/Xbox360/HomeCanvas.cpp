#include "HomeCanvas.h"
#include "BrandLogoPixels.h"
#include "InfinityIconPixels.h"
#include "ControllerButtonPixels.h"
#include "GlyphWidths.h"
#include <algorithm>
#include <cstdio>
#include <cmath>

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
const char* HomeCanvas::ThemeName(unsigned theme){
    static const char* names[]={"Verde Infinity","Azul","Ambar","Violeta","Vermelho","Ciano","Grafite"};
    return names[theme<ThemeCount?theme:0];
}
unsigned HomeCanvas::ThemeColor(unsigned color) const {
    if(!theme_)return color;
    const unsigned r=(color>>16)&255,g=(color>>8)&255,b=color&255;
    // Recolor only green UI accents and green-tinted panels; retain warning colors and artwork.
    if(g<=r||g<=b)return color;
    if(theme_==1){const unsigned low=std::min(r,b),delta=g-low;return (color&0xff000000u)|(low<<16)|((low+delta/2)<<8)|g;}
    if(theme_==2)return (color&0xff000000u)|(g<<16)|(r<<8)|b;
    // Preserve luminance contrast and use only green-dominant UI pixels.
    // Text, artwork, video and non-green semantic warning colors stay intact.
    const unsigned low=std::min(r,b),delta=g-low;
    const auto rgb=[&](unsigned rr,unsigned gg,unsigned bb){return (color&0xff000000u)|(rr<<16)|(gg<<8)|bb;};
    switch(theme_){
        case 3:return rgb(low+delta*3/4,low,g);
        case 4:return rgb(g,low+delta/5,low+delta/4);
        case 5:return rgb(low,g,g);
        case 6:{const unsigned gray=(g+low)/2;return rgb(gray,gray,gray);}
        default:return color;
    }
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
void HomeCanvas::SetAppearance(const AppearanceSettings& appearance,const std::string& path){
    appearance_=appearance.Valid()?appearance:AppearanceSettings{};
    if(path!=wallpaperPath_){ClearWallpaper();wallpaperPath_=path;}
    if(appearance_.background!=AppearanceSettings::Wallpaper||wallpaperTried_)return;
    wallpaperTried_=true;
    if(path.empty())return;
    FILE* file=std::fopen(path.c_str(),"rb");
    if(!file)return;
    std::vector<unsigned char> data(640*360*4);
    const bool ok=std::fread(data.data(),1,data.size(),file)==data.size()&&std::fgetc(file)==EOF&&!std::ferror(file);
    std::fclose(file);
    if(ok)wallpaper_.swap(data);
}
void HomeCanvas::Background() {
    if(appearance_.background==AppearanceSettings::Solid){
        std::fill(frame_.begin(),frame_.end(),ThemeColor(0xff031310u));return;
    }
    if(appearance_.background==AppearanceSettings::Wallpaper&&!wallpaper_.empty()){
        const unsigned light=100-AppearanceSettings::Darkness(appearance_.dim);
        // Fixed 640x360 RGBA, nearest 2x upscaling. Alpha ignored by design.
        for(int y=0;y<360;++y){
            auto* row=&frame_[y*2*Width];
            for(int x=0;x<640;++x){
                const auto* p=&wallpaper_[(y*640+x)*4];
                const unsigned color=0xff000000u|((unsigned(p[0])*light/100)<<16)|((unsigned(p[1])*light/100)<<8)|(unsigned(p[2])*light/100);
                row[x*2]=row[x*2+1]=color;
            }
            std::copy(row,row+Width,row+Width);
        }
        return;
    }
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
void HomeCanvas::RasterIcon(int kind,int x,int y,int size,unsigned tint){
    if(kind<0||kind>=InfinityIconPixels::Count||size<=0||size>256)return;
    tint=ThemeColor(tint);
    const int extent=InfinityIconPixels::Size;
    // Bilinear alpha sampling of stored pixels, no paths or line primitives.
    for(int j=std::max(0,-y);j<size&&y+j<Height;++j){
        const int sy=std::min((j*256+128)*extent/size-128,(extent-1)*256);
        const int v=std::max(0,sy),iy=v/256,fy=v%256,iy1=std::min(iy+1,extent-1);
        for(int i=std::max(0,-x);i<size&&x+i<Width;++i){
            const int sx=std::min((i*256+128)*extent/size-128,(extent-1)*256);
            const int u=std::max(0,sx),ix=u/256,fx=u%256,ix1=std::min(ix+1,extent-1);
            const unsigned a=(InfinityIconPixels::Alpha(kind,ix,iy)*(256-fx)*(256-fy)+InfinityIconPixels::Alpha(kind,ix1,iy)*fx*(256-fy)+InfinityIconPixels::Alpha(kind,ix,iy1)*(256-fx)*fy+InfinityIconPixels::Alpha(kind,ix1,iy1)*fx*fy+32768u)>>16;
            if(a){unsigned& dest=frame_[(y+j)*Width+x+i];dest=blend(tint,dest,std::min(255u,a+a/4));}
        }
    }
}
void HomeCanvas::ControllerButton(int kind,int x,int y,int h){
    if(kind<0||kind>=ControllerButtonPixels::Count||h<=0||h>128)return;
    const bool wide=kind>=4&&kind<=9;
    const int sw=wide?48:32,sx=wide?0:8,w=wide?h*3/2:h;
    // Supersample when reducing; average premultiplied colors to retain transparent edges.
    for(int j=std::max(0,-y);j<h&&j+y<Height;++j)for(int i=std::max(0,-x);i<w&&i+x<Width;++i){
        unsigned a=0,r=0,g=0,b=0;
        for(int yy=0;yy<2;++yy)for(int xx=0;xx<2;++xx){
            const unsigned p=ControllerButtonPixels::Pixel(kind,sx+((i*4+1+2*xx)*sw)/(w*4),((j*4+1+2*yy)*32)/(h*4));
            const unsigned alpha=p>>24;a+=alpha;r+=((p>>16)&255)*alpha;g+=((p>>8)&255)*alpha;b+=(p&255)*alpha;
        }
        if(a){const unsigned rgb=((r/a)<<16)|((g/a)<<8)|(b/a);auto& dest=frame_[(y+j)*Width+x+i];dest=blend(rgb,dest,(a+2)/4);}
    }
}
void HomeCanvas::BrandLogo(int x,int y){
    if(x<0||y<0||x+InfinityBrand::Width>Width||y+InfinityBrand::Height>Height)return;
    for(int j=0;j<InfinityBrand::Height;++j)for(int i=0;i<InfinityBrand::Width;++i){
        const unsigned p=InfinityBrand::Pixel(i,j),a=p>>24;
        if(a){unsigned& d=frame_[(y+j)*Width+x+i];d=blend(p,d,a);}
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
bool HomeCanvas::ProfilePicture(const std::vector<unsigned char>& rgba,int x,int y,int size) {
    if(rgba.size()!=64u*64u*4u||size<=0||size>128||x<0||y<0||
       x>Width-size||y>Height-size)return false;
    bool visible=false;
    for(std::size_t i=3;i<rgba.size();i+=4)if(rgba[i]){visible=true;break;}
    if(!visible)return false;
    for(int j=0;j<size;++j)for(int i=0;i<size;++i){
        const int dx=2*i+1-size,dy=2*j+1-size;
        if(dx*dx+dy*dy>size*size)continue;
        const auto* p=&rgba[4*((j*64/size)*64+i*64/size)];
        if(p[3]){
            auto& dest=frame_[(y+j)*Width+x+i];
            dest=blend((unsigned(p[0])<<16)|(unsigned(p[1])<<8)|p[2],dest,p[3]);
        }
    }
    return true;
}
bool HomeCanvas::AvatarFrame(const std::vector<unsigned char>& rgba,unsigned sw,unsigned sh,int x,int y,int w,int h){
    if(!sw||!sh||sw>256||sh>384||rgba.size()!=std::size_t(sw)*sh*4||
       x<0||y<0||w<=0||h<=0||x>=Width||y>=Height||w>Width-x||h>Height-y)return false;
    bool visible=false;for(std::size_t i=3;i<rgba.size();i+=4)if(rgba[i]){visible=true;break;}
    if(!visible)return false;
    int dw=w,dh=h;
    if(int(sw)*h>w*int(sh))dh=std::max(1,w*int(sh)/int(sw));
    else dw=std::max(1,h*int(sw)/int(sh));
    x+=(w-dw)/2;y+=(h-dh)/2;
    for(int j=0;j<dh;++j)for(int i=0;i<dw;++i){
        const auto* p=&rgba[4*((j*sh/dh)*sw+i*sw/dw)];
        if(p[3]){auto& dest=frame_[(y+j)*Width+x+i];
            dest=blend((unsigned(p[0])<<16)|(unsigned(p[1])<<8)|p[2],dest,p[3]);}
    }
    return true;
}
bool HomeCanvas::LocalArt(const std::string& file,bool cover,int x,int y,int w,int h,bool crop,int radius) {
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
    int cw=sw,ch=sh,sx=0,sy=0;
    if(crop){
        if(sw*h>w*sh){cw=std::max(1,sh*w/h);sx=(sw-cw)/2;}
        else {ch=std::max(1,sw*h/w);sy=cover?0:(sh-ch)/2;}
    }
    radius=std::max(0,std::min(radius,std::min(w,h)/2));
    for(int j=0;j<h;++j){
        int cut=0,edge=std::min(j,h-1-j);
        if(edge<radius){const int dy=radius-edge-1;while(cut<radius&&(radius-cut-1)*(radius-cut-1)+dy*dy>radius*radius)++cut;}
        for(int i=cut;i<w-cut;++i){
            const unsigned char* p=&data[4*((sy+j*ch/h)*sw+sx+i*cw/w)];
            unsigned& dest=frame_[(y+j)*Width+x+i];
            if(p[3])dest=blend((unsigned(p[0])<<16)|(unsigned(p[1])<<8)|p[2],dest,p[3]);
        }
    }
    return true;
}
void HomeCanvas::Text(int x,int y,const std::string& text,int size,unsigned color,int maxWidth) {
    if(art_.empty()||size<=0||maxWidth<=0)return;
    color=ThemeColor(color);
    const int right=x+maxWidth, w=std::max(1,30*size/22),h=std::max(1,34*size/22);
    // Bilinear alpha resampling of the original font atlas, without double-drawing bold glyphs.
    for(size_t pos=0;pos<text.size();){
        unsigned cp=nextGlyph(text,pos);if(cp<32)cp=' ';
        const int index=int(cp)-32,advance=(glyphWidths[index]*size+11)/22;
        if(x+advance>right)break;
        const int sx=index%32*30,sy=234+index/32*34;
        for(int j=std::max(0,-y);j<h&&y+j<Height;++j){
            const int fy=std::max(0,std::min(33*256,((2*j+1)*34*128/h)-128));
            const int iy=fy>>8,ay=fy&255,iy1=std::min(33,iy+1);
            for(int i=std::max(0,-x);i<w&&x+i<Width&&x+i<right;++i){
                const int fx=std::max(0,std::min(29*256,((2*i+1)*30*128/w)-128));
                const int ix=fx>>8,ax=fx&255,ix1=std::min(29,ix+1);
                const unsigned a0=(art_[(sy+iy)*ArtWidth+sx+ix]>>24)*(256-ax)+(art_[(sy+iy)*ArtWidth+sx+ix1]>>24)*ax;
                const unsigned a1=(art_[(sy+iy1)*ArtWidth+sx+ix]>>24)*(256-ax)+(art_[(sy+iy1)*ArtWidth+sx+ix1]>>24)*ax;
                const unsigned alpha=(a0*(256-ay)+a1*ay+32768)>>16;
                if(alpha){auto& dest=frame_[(y+j)*Width+x+i];dest=blend(color,dest,alpha);}
            }
        }
        x+=advance;
    }
}

void HomeCanvas::Stroke(int x0,int y0,int x1,int y1,unsigned color,int width){
    // Antialiased line capsules, with a bounded pixel rectangle.
    if(width<=0)return;
    color=ThemeColor(color);
    const float dx=float(x1-x0),dy=float(y1-y0),length=dx*dx+dy*dy,r=float(width)/2;
    const int pad=width/2+2;
    for(int y=std::max(0,std::min(y0,y1)-pad);y<std::min(int(Height),std::max(y0,y1)+pad+1);++y)
        for(int x=std::max(0,std::min(x0,x1)-pad);x<std::min(int(Width),std::max(x0,x1)+pad+1);++x){
            const float px=float(x-x0),py=float(y-y0);
            const float t=length?std::max(0.0f,std::min(1.0f,(px*dx+py*dy)/length)):0.0f;
            const float ex=px-t*dx,ey=py-t*dy;
            const float coverage=std::max(0.0f,std::min(1.0f,r+0.5f-std::sqrt(ex*ex+ey*ey)));
            if(coverage>0){auto& dest=frame_[y*Width+x];dest=blend(color,dest,unsigned(coverage*255+0.5f));}
        }
}
void HomeCanvas::Shade(int x,int y,int w,int h,unsigned color,unsigned a,unsigned b){
    if(w<=0||h<=0)return;color=ThemeColor(color);a=std::min(a,255u);b=std::min(b,255u);
    for(int j=std::max(0,-y);j<h&&y+j<Height;++j)
        for(int i=std::max(0,-x);i<w&&x+i<Width;++i){
            unsigned alpha=unsigned((int(a)*(w-i)+int(b)*i)/w);
            auto& p=frame_[(y+j)*Width+x+i];p=blend(color,p,alpha);
        }
}
void HomeCanvas::Glass(int x,int y,int w,int h,bool focused){SeriesTile(x,y,w,h,focused,focused?215:190);}
void HomeCanvas::NeonBackground(){
    // Respect explicit solid/wallpaper preferences; default gradient becomes
    // graphite with luminous curved ribbons, computed without external art.
    if(appearance_.background!=AppearanceSettings::Gradient)return;
    for(int y=0;y<Height;++y)for(int x=0;x<Width;++x){
        int wave=Height-36-(x-Width/2)*(x-Width/2)/4600;
        int d=std::abs(y-wave),d2=std::abs(y-wave+27+x/50);
        int glow=std::max(0,36-d)*2+std::max(0,21-d2)*3;
        int glint=std::max(0,3-d)*30;
        unsigned g=unsigned(std::min(255,18+glow+glint)),rr=unsigned(std::min(255,9+glow/5+glint/2)),bb=unsigned(12+glow/12);
        frame_[y*Width+x]=ThemeColor(0xff000000u|(rr<<16)|(g<<8)|bb);
    }
}
void HomeCanvas::CenterText(int x,int y,int w,const std::string& text,int size,unsigned color){
    int width=0;for(size_t i=0;i<text.size();){unsigned cp=nextGlyph(text,i);if(cp<32)cp=' ';width+=(glyphWidths[cp-32]*size+11)/22;}
    Text(x+std::max(0,(w-width)/2),y,text,size,color,w-std::max(0,(w-width)/2));
}


void HomeCanvas::Translucent(int x,int y,int w,int h,unsigned color,unsigned opacity,int radius){
    if(w<=0||h<=0)return;
    radius=std::max(0,std::min(radius,std::min(w,h)/2));
    color=ThemeColor(color);opacity=std::min(opacity,255u);
    for(int j=std::max(0,-y);j<h&&y+j<Height;++j){
        int cut=0,edge=std::min(j,h-1-j);
        if(edge<radius){int dy=radius-edge-1;
            while(cut<radius&&(radius-cut-1)*(radius-cut-1)+dy*dy>radius*radius)++cut;
        }
        for(int i=std::max(cut,-x);i<w-cut&&x+i<Width;++i){
            auto& p=frame_[(y+j)*Width+x+i];p=blend(color,p,opacity);
        }
    }
}
void HomeCanvas::SeriesBackground(){
    if(appearance_.background!=AppearanceSettings::Gradient)return;
    // Smooth dark graphite with three off-center green ribbons like the approved reference.
    for(int y=0;y<Height;++y)for(int x=0;x<Width;++x){
        const int waveBottom=760-x*3/11+(x-690)*(x-690)/7800;
        const int waveRight=570-(x-960)*3/4+(x-1150)*(x-1150)/2100;
        const int waveLeft=110-x*3/5+x*x/2200;
        const int d0=std::abs(y-waveBottom),d1=std::abs(y-waveRight),d2=std::abs(y-waveLeft);
        const int a0=std::max(0,64-d0),a1=std::max(0,52-d1),a2=std::max(0,40-d2);
        const int glow=a0*a0/150+a1*a1/125*std::max(0,x-850)/430+a2*a2/100*std::max(0,300-x)/300;
        const int glint=std::max(0,3-d0)*8+std::max(0,2-d1)*std::max(0,x-1000)/50;
        const unsigned base=unsigned(12+8*(Height-y)/Height);
        const unsigned red=std::min(255u,base+unsigned(glow/7+glint/3));
        const unsigned green=std::min(255u,base+4+unsigned(glow+glint));
        const unsigned blue=std::min(255u,base+1+unsigned(glow/10));
        frame_[y*Width+x]=ThemeColor(0xff000000u|(red<<16)|(green<<8)|blue);
    }
}
void HomeCanvas::SeriesFocus(int x,int y,int w,int h){
    if(w<12||h<12)return;
    const unsigned accent=ThemeColor(0xffa8ff00u);
    const int radius=10;
    for(int yy=std::max(0,y);yy<std::min(int(Height),y+h);++yy)
        for(int xx=std::max(0,x);xx<std::min(int(Width),x+w);++xx){
            const float dx=float(std::max(std::abs(2*(xx-x)-(w-1))-(w-2*radius),0))/2;
            const float dy=float(std::max(std::abs(2*(yy-y)-(h-1))-(h-2*radius),0))/2;
            const float d=std::sqrt(dx*dx+dy*dy);
            const float outer=std::max(0.0f,std::min(1.0f,float(radius)+0.5f-d));
            const float inner=std::max(0.0f,std::min(1.0f,float(radius)-2.0f-d));
            const unsigned alpha=unsigned(std::max(0.0f,outer-inner)*255+0.5f)*(75+focusAnimation_/4)/100;
            if(alpha){auto& p=frame_[yy*Width+xx];p=blend(accent,p,alpha);}
        }
}
void HomeCanvas::SeriesTile(int x,int y,int w,int h,bool focused,unsigned opacity){
    if(w<12||h<12)return;
    Translucent(x,y+5,w,h,0xff000000u,55,11);
    Translucent(x,y,w,h,0xff8a9691u,92,11);
    Translucent(x+1,y+1,w-2,h-2,0xff19221fu,opacity,10);
    // Full-surface shading plus green reflection at lower right.
    const unsigned highlight=ThemeColor(0xff6c7772u),reflection=ThemeColor(0xff426d42u);
    for(int yy=3;yy<h-3;++yy){
        const int inset=(yy<10||yy>=h-10)?9:3;
        const unsigned light=unsigned(43*(h-yy)/h);
        for(int xx=inset;xx<w-inset;++xx){
            if(x+xx<0||x+xx>=Width||y+yy<0||y+yy>=Height)continue;
            auto& dest=frame_[(y+yy)*Width+x+xx];
            dest=blend(highlight,dest,light);
            const unsigned glow=unsigned(45*xx/w)*unsigned(yy)/unsigned(h);
            dest=blend(reflection,dest,glow);
        }
    }
    if(focused){Translucent(x+3,y+3,w-6,h-6,0xff448a18u,38+focusAnimation_*28/100,8);SeriesFocus(x,y,w,h);}
}
void HomeCanvas::DimBackdrop(){
    // Bounded three-sample horizontal softening, in place, no additional full-size buffer.
    for(int y=0;y<Height;++y){
        unsigned prev=frame_[y*Width];
        for(int x=0;x<Width;++x){
            const unsigned current=frame_[y*Width+x];
            const unsigned next=frame_[y*Width+std::min(x+1,int(Width)-1)];
            frame_[y*Width+x]=blend(0xff080b09u,blend(current,blend(prev,next,128),128),165);
            prev=current;
        }
    }
}
void HomeCanvas::TextFit(int x,int y,const std::string& text,int size,unsigned color,int maxWidth){
    if(size<=0||maxWidth<=0)return;
    auto advance=[&](unsigned cp){if(cp<32)cp=' ';return (glyphWidths[cp-32]*size+11)/22;};
    int total=0;for(size_t i=0;i<text.size();)total+=advance(nextGlyph(text,i));
    if(total<=maxWidth){Text(x,y,text,size,color,maxWidth);return;}
    const int dots=3*advance('.');
    if(dots>maxWidth)return;
    int used=0;size_t end=0;
    for(size_t i=0;i<text.size();){const int n=advance(nextGlyph(text,i));if(used+n+dots>maxWidth)break;used+=n;end=i;}
    Text(x,y,text.substr(0,end)+"...",size,color,maxWidth);
}
