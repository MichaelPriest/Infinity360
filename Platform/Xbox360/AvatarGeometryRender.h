#pragma once
#include "AvatarAssetGeometry.h"
#include <cmath>
#include <cstring>
#include <limits>
namespace InfinitySocial {
// CPU diagnostic renderer: original vertex positions and indices, orthographic
// view, flat neutral light and depth. No skinning, materials, or texture decoding.
struct AvatarPoint {float x=0,y=0,z=0;};
inline float AvatarFloat(const unsigned char* p){
    const auto bits=AvatarBE32(p);float v=0;
    static_assert(sizeof(v)==sizeof(bits),"32-bit float required");
    std::memcpy(&v,&bits,sizeof(v));return v;
}
inline float AvatarEdge(const AvatarPoint& a,const AvatarPoint& b,float x,float y){
    return (b.x-a.x)*(y-a.y)-(b.y-a.y)*(x-a.x);
}
inline bool RenderAvatarGeometry(const AvatarMemoryView& cpu,const AvatarMemoryView& gpu,
                                 unsigned w,unsigned h,float yaw,std::vector<unsigned char>& rgba){
    rgba.clear();
    if(!w||!h||w>AvatarFrameMaxWidth||h>AvatarFrameMaxHeight||!std::isfinite(yaw))return false;
    AvatarGeometry geometry;
    if(!ParseAvatarGeometry(cpu,gpu,geometry))return false;
    constexpr std::size_t MaxVertices=100000,MaxTriangles=30000;
    std::size_t nv=0,nt=0;
    for(const auto& m:geometry.meshes){
        if(m.vertexCount>MaxVertices-nv||m.triangles>MaxTriangles-nt)return false;
        nv+=m.vertexCount;nt+=m.triangles;
    }
    const float co=std::cos(yaw),si=std::sin(yaw);
    std::vector<AvatarPoint> positions;positions.reserve(nv);
    std::vector<std::size_t> offsets;
    float minX=std::numeric_limits<float>::infinity(),minY=minX,maxX=-minX,maxY=-minX;
    for(const auto& m:geometry.meshes){
        offsets.push_back(positions.size());
        const auto* bytes=gpu.Read(m.vertices,m.vertexBytes);
        for(unsigned i=0;i<m.vertexCount;++i){
            const auto* p=bytes+std::size_t(i)*m.vertexStride;
            const float x=AvatarFloat(p),y=AvatarFloat(p+4),z=AvatarFloat(p+8);
            if(!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||
               std::abs(x)>10000||std::abs(y)>10000||std::abs(z)>10000)return false;
            AvatarPoint v{x*co+z*si,y,-x*si+z*co};
            minX=std::min(minX,v.x);maxX=std::max(maxX,v.x);
            minY=std::min(minY,v.y);maxY=std::max(maxY,v.y);positions.push_back(v);
        }
    }
    const float dx=maxX-minX,dy=maxY-minY;
    if(dx<1e-5f||dy<1e-5f||w<4||h<4)return false;
    const float scale=0.88f*std::min(float(w-1)/dx,float(h-1)/dy);
    for(auto& v:positions){v.x=(v.x-(minX+maxX)*0.5f)*scale+(w-1)*0.5f;
        v.y=-(v.y-(minY+maxY)*0.5f)*scale+(h-1)*0.5f;v.z*=scale;}
    std::vector<unsigned char> pixels(std::size_t(w)*h*4,0);
    std::vector<float> depth(std::size_t(w)*h,-std::numeric_limits<float>::infinity());
    std::uint64_t work=0;constexpr std::uint64_t WorkLimit=16000000;
    for(std::size_t k=0;k<geometry.meshes.size();++k){
        const auto& m=geometry.meshes[k];const auto* idx=gpu.Read(m.indices,m.indexBytes);
        const auto vertex=[&](std::size_t i)->const AvatarPoint&{
            return positions[offsets[k]+((unsigned(idx[2*i])<<8)|idx[2*i+1])];};
        for(unsigned t=0;t<m.triangles;++t){
            const auto& a=vertex(std::size_t(t)*3);const auto& b=vertex(std::size_t(t)*3+1);
            const auto& c=vertex(std::size_t(t)*3+2);
            const float area=AvatarEdge(a,b,c.x,c.y);
            if(std::abs(area)<1e-5f)continue;
            const int x0=std::max(0,int(std::floor(std::min({a.x,b.x,c.x}))));
            const int y0=std::max(0,int(std::floor(std::min({a.y,b.y,c.y}))));
            const int x1=std::min(int(w)-1,int(std::ceil(std::max({a.x,b.x,c.x}))));
            const int y1=std::min(int(h)-1,int(std::ceil(std::max({a.y,b.y,c.y}))));
            if(x1<x0||y1<y0)continue;
            work+=std::uint64_t(x1-x0+1)*(y1-y0+1);if(work>WorkLimit)return false;
            const float ux=b.x-a.x,uy=b.y-a.y,uz=b.z-a.z;
            const float vx=c.x-a.x,vy=c.y-a.y,vz=c.z-a.z;
            const float nx=uy*vz-uz*vy,ny=uz*vx-ux*vz,nz=ux*vy-uy*vx;
            const float norm=std::sqrt(nx*nx+ny*ny+nz*nz);
            const float light=norm>0?std::min(1.0f,std::abs((0.25f*nx-0.35f*ny+0.9f*nz)/norm)):0;
            const auto shade=static_cast<unsigned char>(75+155*light);
            for(int y=y0;y<=y1;++y)for(int x=x0;x<=x1;++x){
                const float px=x+0.5f,py=y+0.5f;
                const float wa=AvatarEdge(b,c,px,py)/area,wb=AvatarEdge(c,a,px,py)/area,wc=1-wa-wb;
                if(wa<0||wb<0||wc<0)continue;
                const float z=wa*a.z+wb*b.z+wc*c.z;
                const auto i=std::size_t(y)*w+x;
                if(z<=depth[i])continue;
                depth[i]=z;pixels[4*i]=shade;pixels[4*i+1]=shade;pixels[4*i+2]=shade;pixels[4*i+3]=255;
            }
        }
    }
    if(!ValidAvatarFrame(w,h,pixels))return false;
    rgba.swap(pixels);return true;
}
}
