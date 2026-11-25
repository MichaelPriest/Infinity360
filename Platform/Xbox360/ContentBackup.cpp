#include "ContentBackup.h"
#include "StoreSha256.h"
#include "StorageInfo.h"
#include "GameScanner.h"
#include <cstdio>
#include <fstream>
#include <algorithm>
#if !defined(_WIN32)&&!defined(_XBOX)
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <filesystem>
namespace fs=std::filesystem;
#endif
namespace ContentBackup {
std::vector<XboxContentPackage> Staged(const FileWorkspace& ws,const std::string& root,unsigned title){
    std::vector<XboxContentPackage> out;
    if(!title)return out;
    char id[9];std::sprintf(id,"%08X",title);
    const auto folder=FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(root,"UserFiles"),"TitleUpdates"),id);
    std::vector<FolderEntry> rows;
    if(!ws.CanBrowse(folder)||!ReadFolder(folder,rows))return out;
    for(const auto& row:rows){
        if(out.size()>=256)break;
        if(row.directory||row.name.size()<4||row.name.substr(row.name.size()-3)!=".tu"||!FileWorkspace::ValidName(row.name))continue;
        auto path=FileWorkspace::Join(folder,row.name);XboxContentPackage p{};
        if(ws.CanReadFile(path)&&ContentCatalog::ReadHeader(path,p)&&p.titleId==title&&p.contentType==0xb0000&&p.volumeType==0){
            p.name=row.name;out.push_back(p);
        }
    }
    return out;
}
bool Archive(const FileWorkspace& ws,const std::string& root,const XboxContentPackage& selected,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error){
    error.clear();
#if defined(_WIN32)||defined(_XBOX)
    (void)ws;(void)root;(void)selected;(void)progress;error="Provider indisponivel.";return false;
#else
    auto reject=[&](const char* msg){error=msg;return false;};
    std::ifstream marker(FileWorkspace::Join(root,"enable-content-backup.txt").c_str());std::string signature;
    if(!std::getline(marker,signature)||signature!="ENABLE-CONTENT-BACKUP")return reject("Habilite enable-content-backup.txt.");
    XboxContentPackage current{};
    if(!ws.CanReadFile(selected.path)||!ContentCatalog::ReadHeader(selected.path,current)||current.titleId!=selected.titleId||
       current.contentType!=selected.contentType||current.mediaId!=selected.mediaId||current.version!=selected.version||
       current.volumeType!=0||(current.contentType!=1&&current.contentType!=2&&current.contentType!=0xb0000))return reject("Conteudo mudou ou formato nao suportado.");
    std::ifstream input(selected.path.c_str(),std::ios::binary|std::ios::ate);
    if(!input||input.tellg()<0x1711||input.tellg()>512LL*1024*1024)return reject("Tamanho invalido (max 512 MiB).");
    const auto size=static_cast<unsigned long long>(input.tellg());input.seekg(0);
    const auto personal=FileWorkspace::Join(root,"UserFiles"),back=FileWorkspace::Join(personal,"ContentBackups");
    const auto space=StorageInfo::Read(personal);
    if(!ws.CanWrite(personal)||!space.available||space.writableBytes<size+16ULL*1024*1024||
       (!ws.CanWrite(back)&&!ws.Create(personal,"ContentBackups")))return reject("Area pessoal ou espaco insuficiente.");
    char title[9];std::sprintf(title,"%08X",selected.titleId);
    const auto folder=FileWorkspace::Join(back,title);
    if(!ws.CanWrite(folder)&&!ws.Create(back,title))return reject("Pasta backup indisponivel.");
    char finalName[40];std::string stage;
    for(unsigned i=0;i<32&&stage.empty();++i){
        std::sprintf(finalName,"%08X-%04u",selected.contentType,i);
        if(ws.BeginDiscStage(folder,finalName,stage))break;
    }
    if(stage.empty())return reject("Limite de backups ou pasta indisponivel.");
    const auto file=FileWorkspace::Join(stage,"content.bin"),receipt=FileWorkspace::Join(stage,"backup.txt");
    int fd=-1;auto fail=[&](const char* msg){
        if(fd>=0)close(fd);unlink(file.c_str());unlink(receipt.c_str());std::error_code ec;fs::remove(stage,ec);error=msg;return false;
    };
    int flags=O_WRONLY|O_CREAT|O_EXCL;
#ifndef __XSDK__
    flags|=O_NOFOLLOW;
#endif
    fd=open(file.c_str(),flags,0600);if(fd<0)return fail("Falha ao reservar backup.");
    StoreSha256 sha;unsigned char buf[32768];unsigned long long done=0;
    if(!progress(0,size*2))return fail("Cancelado.");
    while(done<size){
        const size_t n=static_cast<size_t>(std::min<unsigned long long>(sizeof buf,size-done));
        if(!input.read(reinterpret_cast<char*>(buf),n))return fail("Falha na leitura.");
        size_t pos=0;while(pos<n){const ssize_t w=write(fd,buf+pos,n-pos);if(w<0&&errno==EINTR)continue;if(w<=0)return fail("Falha na escrita.");pos+=static_cast<size_t>(w);}
        sha.Update(buf,n);done+=n;if(!progress(done,size*2))return fail("Cancelado.");
    }
    if(input.peek()!=std::char_traits<char>::eof())return fail("Origem alterada.");
    const auto digest=sha.Finish();const bool synced=fsync(fd)==0,closed=close(fd)==0;fd=-1;
    if(!synced||!closed)return fail("Falha ao sincronizar.");
    StoreSha256 check;std::ifstream copied(file.c_str(),std::ios::binary);
    for(unsigned long long pos=0;pos<size;){
        const size_t n=static_cast<size_t>(std::min<unsigned long long>(sizeof buf,size-pos));
        if(!copied.read(reinterpret_cast<char*>(buf),n))return fail("Falha ao verificar.");
        check.Update(buf,n);pos+=n;if(!progress(size+pos,size*2))return fail("Cancelado.");
    }
    if(check.Finish()!=digest||copied.peek()!=std::char_traits<char>::eof())return fail("SHA-256 divergente.");
    copied.close();XboxContentPackage verify{};
    if(!ContentCatalog::ReadHeader(file,verify)||verify.titleId!=current.titleId||verify.contentType!=current.contentType||
       verify.mediaId!=current.mediaId||verify.version!=current.version)return fail("Cabecalho divergente.");
    if(selected.path.find_first_of("\r\n\t")!=std::string::npos)return fail("Caminho invalido.");
    const std::string record="INFINITY-CONTENT-BACKUP-1\n"+digest+"\n"+std::to_string(size)+"\n"+selected.path+"\n";
    fd=open(receipt.c_str(),flags,0600);if(fd<0)return fail("Falha ao criar registro.");
    const bool saved=write(fd,record.data(),record.size())==static_cast<ssize_t>(record.size());
    const bool flushed=fsync(fd)==0,finished=close(fd)==0;fd=-1;
    if(!saved||!flushed||!finished||!ws.PublishDiscStage(folder,stage,finalName))return fail("Falha ao publicar backup.");
    return true;
#endif
}
}
