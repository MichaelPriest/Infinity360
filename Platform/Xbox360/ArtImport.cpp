#include "ArtImport.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#if !defined(_WIN32)&&!defined(_XBOX)
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#endif
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_SIMD
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_MAX_DIMENSIONS 2048
#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb/stb_image.h"
namespace ArtImport {
bool Convert(const std::string& encoded,bool cover,std::vector<unsigned char>& rgba){
    rgba.clear();if(encoded.empty()||encoded.size()>4*1024*1024)return false;
    int w=0,h=0,c=0;const auto* bytes=reinterpret_cast<const unsigned char*>(encoded.data());
    if(!stbi_info_from_memory(bytes,static_cast<int>(encoded.size()),&w,&h,&c)||w<1||h<1||w>2048||h>2048)return false;
    const int ew=w,eh=h;
    auto* image=stbi_load_from_memory(bytes,static_cast<int>(encoded.size()),&w,&h,&c,4);
    if(!image)return false;
    if(w!=ew||h!=eh){stbi_image_free(image);return false;}
    const int dw=cover?160:640,dh=cover?240:360;
    int cw=w,ch=h;
    if(w*dh>h*dw)cw=std::max(1,h*dw/dh);else ch=std::max(1,w*dh/dw);
    const int left=(w-cw)/2,top=(h-ch)/2;
    rgba.resize(dw*dh*4);
    for(int y=0;y<dh;++y)for(int x=0;x<dw;++x){
        const auto* p=image+4*((top+y*ch/dh)*w+left+x*cw/dw);
        std::copy(p,p+4,rgba.begin()+4*(y*dw+x));
    }
    stbi_image_free(image);return true;
}
std::string Folder(const std::string& root,unsigned title){
    if(!title)return "";char id[9];std::sprintf(id,"%08X",title);
    return FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(root,"UserFiles"),"Art"),id);
}
bool Save(const FileWorkspace& ws,const std::string& root,unsigned title,bool cover,const std::string& encoded,std::string& error){
    error.clear();
#if defined(_WIN32)||defined(_XBOX)
    (void)ws;(void)root;(void)title;(void)cover;(void)encoded;error="Provider indisponivel.";return false;
#else
    auto reject=[&](const char* msg){error=msg;return false;};
    if(!title)return reject("Title ID desconhecido.");
    std::vector<unsigned char> pixels;if(!Convert(encoded,cover,pixels))return reject("Imagem PNG/JPEG invalida ou grande.");
    const auto personal=FileWorkspace::Join(root,"UserFiles"),art=FileWorkspace::Join(personal,"Art"),folder=Folder(root,title);
    char id[9];std::sprintf(id,"%08X",title);
    if(!ws.CanWrite(personal)||(!ws.CanWrite(art)&&!ws.Create(personal,"Art"))||
       (!ws.CanWrite(folder)&&!ws.Create(art,id)))return reject("Area pessoal de arte indisponivel.");
    const auto dest=FileWorkspace::Join(folder,cover?"cover.rgba":"background.rgba"),temp=FileWorkspace::Join(folder,"AXCOPY-ART.part");
    if(ws.CanReadPersonalFile(dest))return reject("Arte pessoal ja existe; arquive pelo explorer antes de substituir.");
    int flags=O_WRONLY|O_CREAT|O_EXCL;
#ifndef __XSDK__
    flags|=O_NOFOLLOW;
#endif
    int fd=open(temp.c_str(),flags,0600);if(fd<0)return reject("Temporario indisponivel.");
    size_t pos=0;while(pos<pixels.size()){
        const ssize_t n=write(fd,pixels.data()+pos,pixels.size()-pos);
        if(n<0&&errno==EINTR)continue;if(n<=0)break;pos+=static_cast<size_t>(n);
    }
    const bool synced=fsync(fd)==0,closed=close(fd)==0;
    bool published=false;
    if(pos==pixels.size()&&synced&&closed){
#ifdef __XSDK__
        published=ws.PublishStaged(temp,dest);
#else
        published=link(temp.c_str(),dest.c_str())==0;
#endif
    }
    unlink(temp.c_str());if(!published)return reject("Falha ao salvar sem sobrescrever.");return true;
#endif
}
bool Local(const FileWorkspace& ws,const std::string& root,unsigned title,bool cover,std::string& error){
    if(!title){error="Title ID desconhecido.";return false;}
    char id[9];std::sprintf(id,"%08X",title);
    const auto folder=FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(root,"UserFiles"),"ArtImport"),id);
    for(const char* ext:{".jpg",".png"}){
        const auto path=FileWorkspace::Join(folder,std::string(cover?"cover":"background")+ext);
        if(!ws.CanReadPersonalFile(path))continue;
        std::ifstream input(path.c_str(),std::ios::binary|std::ios::ate);
        if(!input||input.tellg()<=0||input.tellg()>4*1024*1024){error="Imagem local excede limite.";return false;}
        std::string data(static_cast<size_t>(input.tellg()),'\0');input.seekg(0);
        if(!input.read(&data[0],data.size())){error="Falha ao ler imagem.";return false;}
        return Save(ws,root,title,cover,data,error);
    }
    error="Coloque JPG/PNG em UserFiles/ArtImport/<TitleID>.";return false;
}
}
