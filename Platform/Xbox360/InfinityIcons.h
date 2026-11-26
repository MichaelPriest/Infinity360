#pragma once
#include "HomeCanvas.h"
namespace InfinityIcons {
// Shared raster image atlas. IDs 0–27 retain compatibility; 28–31 add
// sun, moon, rain and neutral profile. Tint affects UI state, not geometry.
enum { Sun=28, Moon=29, Rain=30, Avatar=31, Folder=32, Search, Pin, Package, Update, Dlc, Save, Backup, Archive, Trainer, Plugin, Thermometer, Fan, Language, Display, Audio, Server, Shield, Check, Warning, Film, Series, Animation, Trash, Cut, Copy, Sort, Refresh, Info, Globe, Image, Usb, Count };
inline void Draw(HomeCanvas& c,int kind,int x,int y,int size,unsigned color){
    c.RasterIcon(kind,x,y,size,color);
}
}
