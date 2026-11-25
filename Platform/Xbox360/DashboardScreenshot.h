#pragma once
#include "HomeCanvas.h"
#include "FileWorkspace.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#if defined(__XSDK__) || defined(__linux__)
#include <fcntl.h>
#include <unistd.h>
#endif

// BMP stores explicit little-endian bytes; never serialize native structs on PPC.
// Capture only the dashboard's CPU canvas, not the GPU or another title.
namespace DashboardScreenshot {
constexpr unsigned Width=HomeCanvas::Width,Height=HomeCanvas::Height,RowBytes=(Width*3+3)&~3u;
constexpr unsigned FileBytes=54+RowBytes*Height;
inline void Put16(unsigned char* p,uint16_t n){p[0]=n&255;p[1]=n>>8;}
inline void Put32(unsigned char* p,uint32_t n){for(unsigned i=0;i<4;++i)p[i]=(n>>(8*i))&255;}
template<class Write> bool Encode(const unsigned* pixels,Write write){
    if(!pixels)return false;
    std::array<unsigned char,54> h{};
    h[0]='B';h[1]='M';Put32(h.data()+2,FileBytes);Put32(h.data()+10,54);
    Put32(h.data()+14,40);Put32(h.data()+18,Width);Put32(h.data()+22,Height);
    Put16(h.data()+26,1);Put16(h.data()+28,24);Put32(h.data()+34,RowBytes*Height);
    if(!write(h.data(),h.size()))return false;
    std::array<unsigned char,RowBytes> row{};
    for(unsigned y=Height;y>0;--y){
        for(unsigned x=0;x<Width;++x){
            const unsigned v=pixels[(y-1)*Width+x];
            row[x*3]=v&255;row[x*3+1]=(v>>8)&255;row[x*3+2]=(v>>16)&255;
        }
        if(!write(row.data(),row.size()))return false;
    }
    return true;
}
inline std::string Save(const HomeCanvas& canvas,const FileWorkspace& workspace,const std::string& root){
#if defined(__XSDK__) || defined(__linux__)
    const auto personal=workspace.WorkspaceFor(root);
    if(personal.empty()||(!workspace.CanWrite(personal)&&!workspace.Create(root,"UserFiles")))return {};
    const auto directory=FileWorkspace::Join(personal,"Screenshots");
    if(!workspace.CanWrite(directory)&&!workspace.Create(personal,"Screenshots"))return {};
    if(!workspace.CanWrite(directory))return {};
    // Reserve a temporary file exclusively, then publish via no-replace move.
    for(unsigned i=1;i<=9999;++i){
        char name[24];std::snprintf(name,sizeof name,"Shot%04u.bmp",i);
        const auto destination=FileWorkspace::Join(directory,name);
        if(workspace.CanReadPersonalFile(destination))continue;
        const auto temporary=FileWorkspace::Join(directory,std::string(name)+".part");
        int fd=open(temporary.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
        if(fd<0)continue;
        const bool encoded=Encode(canvas.Pixels(),[&](const unsigned char* data,size_t size){
            while(size){const auto n=write(fd,data,size);if(n<=0)return false;data+=n;size-=static_cast<size_t>(n);}return true;
        });
        const bool closed=close(fd)==0;
        if(encoded&&closed&&workspace.MovePersonalExclusive(temporary,destination,false))return destination;
        std::remove(temporary.c_str());
        return {};
    }
#else
    (void)canvas;(void)workspace;(void)root;
#endif
    return {};
}
}
