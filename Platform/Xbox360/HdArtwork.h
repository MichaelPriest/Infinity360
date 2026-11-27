#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <vector>
#include <string>

namespace HdArtwork {
struct Size { unsigned width,height; std::size_t Bytes()const{return std::size_t(width)*height*4;} };
constexpr Size LegacyCover{160,240}, HdCover{480,720}, LegacyBackdrop{640,360}, HdBackdrop{1280,720};
inline bool Resolve(std::size_t bytes,bool cover,Size& size){
    const Size low=cover?LegacyCover:LegacyBackdrop,high=cover?HdCover:HdBackdrop;
    if(bytes==low.Bytes()){size=low;return true;}
    if(bytes==high.Bytes()){size=high;return true;}
    return false;
}
// Only the two documented raw RGBA dimensions are allowed.
inline bool Read(const std::string& path,bool cover,std::vector<unsigned char>& data,Size& size){
    data.clear();
    FILE* file=std::fopen(path.c_str(),"rb");if(!file)return false;
    bool ok=std::fseek(file,0,SEEK_END)==0;
    const long length=ok?std::ftell(file):-1;
    ok=length>0&&Resolve(static_cast<std::size_t>(length),cover,size)&&std::fseek(file,0,SEEK_SET)==0;
    if(ok){data.resize(size.Bytes());ok=std::fread(data.data(),1,data.size(),file)==data.size()&&std::fgetc(file)==EOF&&!std::ferror(file);}
    std::fclose(file);
    if(!ok)std::vector<unsigned char>().swap(data);
    return ok;
}
inline unsigned Coordinate(unsigned dst,unsigned source,unsigned extent){
    if(extent<=1)return (source-1)*128;
    const int coordinate=int((2ull*dst+1)*source*128/extent)-128;
    return unsigned(std::max(0,std::min(coordinate,int((source-1)*256))));
}
// Interpolate in premultiplied alpha to avoid dark fringes at transparent edges.
inline unsigned Sample(const unsigned char* data,unsigned stride,unsigned sw,unsigned sh,unsigned x,unsigned y,unsigned dw,unsigned dh){
    const unsigned u=Coordinate(x,sw,dw),v=Coordinate(y,sh,dh);
    const unsigned ix=u>>8,iy=v>>8,ix1=std::min(ix+1,sw-1),iy1=std::min(iy+1,sh-1),fx=u&255,fy=v&255;
    const unsigned weights[]={(256-fx)*(256-fy),fx*(256-fy),(256-fx)*fy,fx*fy};
    const unsigned char* p[]={data+4*(iy*stride+ix),data+4*(iy*stride+ix1),data+4*(iy1*stride+ix),data+4*(iy1*stride+ix1)};
    unsigned a=0,r=0,g=0,b=0;
    for(unsigned i=0;i<4;++i){const unsigned k=p[i][3]*weights[i];a+=k;r+=p[i][0]*k;g+=p[i][1]*k;b+=p[i][2]*k;}
    if(!a)return 0;
    return ((a+32768u)>>16)<<24|((r+a/2)/a)<<16|((g+a/2)/a)<<8|((b+a/2)/a);
}
inline void Write(unsigned char* out,unsigned argb){out[0]=argb>>16;out[1]=argb>>8;out[2]=argb;out[3]=argb>>24;}
}
