#include "UnityStage.h"
#include "StorageInfo.h"
#include "StoreSha256.h"
#include "ContentInstall.h"
#include <mbedtls/md5.h>
#include <cstdio>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
bool UnityStageUpdate(const FileWorkspace& ws,const XboxExecutionInfo& info,const UnityUpdate& item,
    const std::string& md5,const std::string& root,
    const std::function<bool(const std::function<bool(const unsigned char*,size_t)>&)>& download,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error){
    error.clear();
    auto reject=[&](const char* message){error=message;return false;};
    if(!info.titleId||!info.mediaId||!info.baseVersion||!item.id||item.bytes<0x1711||item.bytes>128ULL*1024*1024||
       md5.size()!=32||md5.find_first_not_of("0123456789abcdef")!=std::string::npos)return reject("Identidade, tamanho ou MD5 indisponivel.");
    std::ifstream marker(FileWorkspace::Join(root,"enable-xboxunity.txt").c_str());std::string line;
    if(!std::getline(marker,line)||line!="ENABLE-XBOXUNITY")return reject("XboxUnity desabilitado.");
    const auto personal=FileWorkspace::Join(root,"UserFiles");
    if(!ws.CanWrite(personal))return reject("Habilite escrita na area pessoal.");
    const auto storage=StorageInfo::Read(personal);
    if(!storage.available||storage.writableBytes<item.bytes+16ULL*1024*1024)return reject("Espaco livre insuficiente.");
    char title[9],filename[32];std::sprintf(title,"%08X",info.titleId);std::sprintf(filename,"%08X-%u.tu",info.mediaId,item.id);
    std::string folder=personal;
    for(const auto& name:std::vector<std::string>{"TitleUpdates",title}){
        const auto next=FileWorkspace::Join(folder,name);
        if(!ws.CanWrite(next)&&!ws.Create(folder,name))return reject("Pasta TU indisponivel.");
        folder=next;
    }
    const auto dest=FileWorkspace::Join(folder,filename);
    if(ws.CanReadPersonalFile(dest))return reject("TU local existente; nao sobrescrita.");
    const auto temp=FileWorkspace::Join(folder,"AXCOPY-TU.part");
    int flags=O_WRONLY|O_CREAT|O_EXCL;
#ifndef __XSDK__
    flags|=O_NOFOLLOW;
#endif
    int fd=open(temp.c_str(),flags,0600);
    if(fd<0)return reject("Temporario existente ou indisponivel.");
    auto fail=[&](const char* message){if(fd>=0)close(fd);unlink(temp.c_str());error=message;return false;};
    StoreSha256 sha;
    mbedtls_md5_context hash;mbedtls_md5_init(&hash);
    unsigned long long done=0;
    bool valid=mbedtls_md5_starts(&hash)==0&&progress(0,item.bytes);
    if(valid)valid=download([&](const unsigned char* bytes,size_t n){
        if(n>32768||n>item.bytes-done)return false;
        size_t pos=0;while(pos<n){const ssize_t wrote=write(fd,bytes+pos,n-pos);if(wrote<0&&errno==EINTR)continue;if(wrote<=0)return false;pos+=static_cast<size_t>(wrote);}
        if(mbedtls_md5_update(&hash,bytes,n)!=0)return false;
        sha.Update(bytes,n);done+=n;return progress(done,item.bytes);
    });
    unsigned char digest[16];const bool hashed=mbedtls_md5_finish(&hash,digest)==0;
    mbedtls_md5_free(&hash);
    char hex[33];if(hashed)for(unsigned i=0;i<16;++i)std::sprintf(hex+i*2,"%02x",digest[i]);
    if(!valid||!hashed||done!=item.bytes||md5!=hex)return fail("Download cancelado, incompleto ou MD5 divergente.");
    const bool synced=fsync(fd)==0;const bool closed=close(fd)==0;fd=-1;
    if(!synced||!closed)return fail("Falha ao sincronizar TU.");
    XboxContentPackage package{};
    if(!ContentCatalog::ReadHeader(temp,package)||package.contentType!=0xb0000||package.titleId!=info.titleId||
       package.mediaId!=info.mediaId||package.volumeType!=0)return fail("TU incompativel com Title ID, Media ID ou formato.");
#ifdef __XSDK__
    if(!ws.PublishStaged(temp,dest))return fail("Falha ao publicar sem sobrescrever.");
#else
    if(link(temp.c_str(),dest.c_str())!=0)return fail("Falha ao publicar sem sobrescrever.");
    unlink(temp.c_str());
#endif
    // Keep the original filename and SHA-256 for a separate, explicit install.
    // Missing/legacy filenames still allow staging; never invent a system filename.
    if(ContentInstall::Filename(0xb0000u,item.filename)){
        const auto receipt=dest+".receipt",receiptTemp=FileWorkspace::Join(folder,"AXCOPY-TU-RECEIPT.part");
        const std::string record="INFINITY-TU-1\n"+std::to_string(info.baseVersion)+"\n"+item.filename+"\n"+std::to_string(item.bytes)+"\n"+sha.Finish()+"\n";
        int out=open(receiptTemp.c_str(),flags,0600);
        if(out<0){error="TU baixada; recibo indisponivel. Instalacao manual necessaria.";return true;}
        size_t pos=0;while(pos<record.size()){
            const ssize_t wrote=write(out,record.data()+pos,record.size()-pos);
            if(wrote<0&&errno==EINTR)continue;if(wrote<=0)break;pos+=static_cast<size_t>(wrote);
        }
        const bool flushed=fsync(out)==0,finished=close(out)==0;
        bool published=false;
        if(pos==record.size()&&flushed&&finished){
#ifdef __XSDK__
            published=ws.PublishStaged(receiptTemp,receipt);
#else
            published=link(receiptTemp.c_str(),receipt.c_str())==0;
#endif
        }
        unlink(receiptTemp.c_str());
        if(!published)error="TU baixada; recibo nao publicado. Instalacao manual necessaria.";
    }else error="TU baixada; nome antigo ou desconhecido. Instalacao direta indisponivel.";
    return true;
}
