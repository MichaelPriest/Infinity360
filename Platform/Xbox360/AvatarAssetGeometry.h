#pragma once
#include "AvatarManifest.h"

namespace InfinitySocial {
// Independent bounded reader of the observed 32-bit big-endian XAM asset ABI.
// Guest addresses are offsets into two explicit owned memory regions, never
// converted into native pointers. This is not a native renderer.
struct AvatarBufferPlan {
    bool valid=false;
    std::uint32_t cpuBytes=0,gpuBytes=0;
};
constexpr std::uint32_t AvatarGpuAlignment=65536,AvatarCpuAlignment=128;
inline AvatarBufferPlan PlanAvatarBuffers(const AvatarAssetBudget& b){
    if(b.state!=AvatarBudgetState::Ready||!b.cpu||!b.gpu||
       b.cpu>AvatarCpuLimit||b.gpu>AvatarGpuLimit)return {};
    const auto round=[](std::uint32_t n,std::uint32_t a){return (n+a-1u)&~(a-1u);};
    return {true,round(b.cpu,AvatarCpuAlignment),round(b.gpu,AvatarGpuAlignment)};
}
struct AvatarMemoryView {
    const unsigned char* data=nullptr;
    std::uint32_t base=0;
    std::size_t bytes=0;
    bool Valid(std::size_t limit)const{
        return data&&base&&bytes&&bytes<=limit&&std::uint64_t(base)+bytes<=0x100000000ull;
    }
    const unsigned char* Read(std::uint32_t address,std::size_t length)const{
        if(!data||!address||address<base||!length)return nullptr;
        const auto offset=std::size_t(address-base);
        if(offset>bytes||length>bytes-offset)return nullptr;
        return data+offset;
    }
};
struct AvatarMeshRange {
    std::uint32_t vertices=0,vertexBytes=0,indices=0,indexBytes=0;
    std::uint32_t triangles=0,vertexCount=0,uvCount=0,vertexStride=0,indexStride=0;
};
struct AvatarGeometry {
    unsigned joints=0,components=0,textures=0;
    std::vector<AvatarMeshRange> meshes;
};
constexpr unsigned AvatarMaxJoints=256,AvatarMaxModels=32,AvatarMaxBatches=256,AvatarMaxTextures=128;
// The official structure has a separate prop pointer. Until its independent
// layout is verified this conservative reader rejects props, not silently
// treating them as ordinary clothing or exposing a partial result.
inline bool ParseAvatarGeometry(const AvatarMemoryView& cpu,const AvatarMemoryView& gpu,AvatarGeometry& out){
    out={};
    if(!cpu.Valid(AvatarCpuLimit)||!gpu.Valid(AvatarGpuLimit))return false;
    if(std::uint64_t(cpu.base)<std::uint64_t(gpu.base)+gpu.bytes&&
       std::uint64_t(gpu.base)<std::uint64_t(cpu.base)+cpu.bytes)return false;
    const auto* root=cpu.Read(cpu.base,20);
    if(!root||AvatarBE32(root+4))return false;
    const auto count=AvatarBE32(root+8);
    if(!count||count>AvatarMaxModels)return false;
    const auto* skeleton=cpu.Read(AvatarBE32(root),8);
    if(!skeleton)return false;
    const auto joints=AvatarBE32(skeleton);
    if(!joints||joints>AvatarMaxJoints||!cpu.Read(AvatarBE32(skeleton+4),std::size_t(joints)*96u))return false;
    if(!cpu.Read(AvatarBE32(root+12),std::size_t(count)*32u))return false;
    const auto* models=cpu.Read(AvatarBE32(root+16),std::size_t(count)*52u);
    if(!models)return false;
    AvatarGeometry result;result.joints=joints;result.components=count;
    for(unsigned i=0;i<count;++i){
        const auto* m=models+i*52u;
        const auto vb=AvatarBE32(m+12),ib=AvatarBE32(m+16);
        const auto batchCount=AvatarBE32(m+20),textureCount=AvatarBE32(m+24);
        if(batchCount>AvatarMaxBatches-result.meshes.size()||
           textureCount>AvatarMaxTextures-result.textures)return false;
        if(vb&&!gpu.Read(AvatarBE32(m+36),vb))return false;
        if(ib&&!gpu.Read(AvatarBE32(m+40),ib))return false;
        if(batchCount){
            const auto* batches=cpu.Read(AvatarBE32(m+44),std::size_t(batchCount)*512u);
            if(!batches||!vb||!ib)return false;
            for(unsigned j=0;j<batchCount;++j){
                const auto* b=batches+j*512u;
                AvatarMeshRange mesh;
                mesh.triangles=AvatarBE32(b+484);mesh.vertexCount=AvatarBE32(b+488);
                mesh.uvCount=AvatarBE32(b+492);mesh.vertexStride=AvatarBE32(b+496);
                mesh.indexStride=AvatarBE32(b+500);mesh.vertices=AvatarBE32(b+504);
                mesh.indices=AvatarBE32(b+508);
                if(!mesh.triangles||!mesh.vertexCount||mesh.vertexCount>65535||
                   mesh.uvCount>8||mesh.vertexStride<28u+4u*mesh.uvCount||
                   mesh.vertexStride>256||mesh.indexStride!=2)return false;
                const auto vertexBytes=std::uint64_t(mesh.vertexCount)*mesh.vertexStride;
                const auto indexBytes=std::uint64_t(mesh.triangles)*3u*mesh.indexStride;
                if(vertexBytes>vb||indexBytes>ib)return false;
                mesh.vertexBytes=static_cast<std::uint32_t>(vertexBytes);
                mesh.indexBytes=static_cast<std::uint32_t>(indexBytes);
                if(!gpu.Read(mesh.vertices,mesh.vertexBytes)||!gpu.Read(mesh.indices,mesh.indexBytes))return false;
                // A batch must point inside its owning model's global buffers.
                const auto vbase=AvatarBE32(m+36),ibase=AvatarBE32(m+40);
                if(mesh.vertices<vbase||mesh.indices<ibase||
                   std::uint64_t(mesh.vertices)+mesh.vertexBytes>std::uint64_t(vbase)+vb||
                   std::uint64_t(mesh.indices)+mesh.indexBytes>std::uint64_t(ibase)+ib)return false;
                const auto* indices=gpu.Read(mesh.indices,mesh.indexBytes);
                for(std::size_t k=0;k<mesh.indexBytes;k+=2)
                    if(((unsigned(indices[k])<<8)|indices[k+1])>=mesh.vertexCount)return false;
                result.meshes.push_back(mesh);
            }
        }
        if(textureCount){
            const auto* textures=cpu.Read(AvatarBE32(m+48),std::size_t(textureCount)*44u);
            if(!textures)return false;
            for(unsigned j=0;j<textureCount;++j){
                const auto* t=textures+j*44u;
                const auto w=AvatarBE32(t+4),h=AvatarBE32(t+8);
                const auto baseBytes=AvatarBE32(t+12),mipBytes=AvatarBE32(t+16);
                if(!w||!h||w>4096||h>4096||!baseBytes||
                   !gpu.Read(AvatarBE32(t+36),baseBytes)||
                   (mipBytes&&!gpu.Read(AvatarBE32(t+40),mipBytes)))return false;
            }
            result.textures+=textureCount;
        }
    }
    if(result.meshes.empty())return false;
    out=std::move(result);return true;
}
}
