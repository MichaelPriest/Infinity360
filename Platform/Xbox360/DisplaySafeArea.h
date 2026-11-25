#pragma once
#include <algorithm>
#include <string>
#include <vector>
#include "HomeCanvas.h"
// Uniform inset preserves geometry. Software-only compensation for cropped edges.
namespace DisplaySafeArea {
constexpr unsigned Max=10;
inline bool Parse(const std::string& text,unsigned& out){
    if(text.empty()||text.size()>2)return false;
    unsigned n=0;for(char c:text){if(c<'0'||c>'9')return false;n=n*10+c-'0';}
    if(n>Max)return false;out=n;return true;
}
inline const unsigned* Apply(const unsigned* source,unsigned percent,std::vector<unsigned>& scratch){
    if(!source)return nullptr;
    percent=std::min(percent,Max);
    if(!percent)return source;
    constexpr unsigned w=HomeCanvas::Width,h=HomeCanvas::Height;
    const unsigned mx=w*percent/100,my=h*percent/100,iw=w-2*mx,ih=h-2*my;
    scratch.assign(w*h,0xff000000u);
    for(unsigned y=0;y<ih;++y){
        const unsigned* row=source+(y*h/ih)*w;
        unsigned* dest=scratch.data()+(my+y)*w+mx;
        for(unsigned x=0;x<iw;++x)dest[x]=row[x*w/iw];
    }
    return scratch.data();
}
}
