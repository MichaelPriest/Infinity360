#pragma once
#include "HomeCanvas.h"
namespace InfinityIcons {
// Shared raster image atlas. IDs 0–27 retain compatibility; 28–31 add
// sun, moon, rain and neutral profile. Tint affects UI state, not geometry.
enum { Sun=28, Moon=29, Rain=30, Avatar=31, Count=32 };
inline void Draw(HomeCanvas& c,int kind,int x,int y,int size,unsigned color){
    c.RasterIcon(kind,x,y,size,color);
}
}
