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
namespace {
std::string titleFolder(const std::string& root,const char* area,unsigned title){
    char id[9];std::sprintf(id,"%08X",title);
    return FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(root,"UserFiles"),area),id);
}
bool enabled(const std::string& root){
    std::ifstream f(FileWorkspace::Join(root,"enable-content-backup.txt").c_str());std::string line;
    return bool(std::getline(f,line))&&line=="ENABLE-CONTENT-BACKUP";
}
bool same(const XboxContentPackage& a,const XboxContentPackage& b){
    return a.titleId==b.titleId&&a.mediaId==b.mediaId&&a.version==b.version&&a.contentType==b.contentType&&a.volumeType==b.volumeType&&a.baseVersion==b.baseVersion&&a.discNumber==b.discNumber&&a.discCount==b.discCount&&a.sizeBytes==b.sizeBytes&&a.dataFileCount==b.dataFileCount;
}
bool record(const FileWorkspace& ws,const std::string& root,unsigned title,const std::string& slot,Record& out){
    if(!title||slot.size()!=13||slot[8]!='-'||slot.substr(9).find_first_not_of("0123456789")!=std::string::npos||slot.substr(0,8).find_first_not_of("0123456789ABCDEF")!=std::string::npos)return false;
    const auto folder=FileWorkspace::Join(titleFolder(root,"ContentBackups",title),slot);
    const auto receipt=FileWorkspace::Join(folder,"backup.txt"),file=FileWorkspace::Join(folder,"content.bin");
    if(!ws.CanReadFile(receipt)||!ws.CanReadFile(file))return false;
    std::ifstream input(receipt.c_str(),std::ios::binary|std::ios::ate);
    if(!input||input.tellg()<1||input.tellg()>1024)return false;
    input.seekg(0);std::string magic,sha,size,original,extra;
    if(!std::getline(input,magic)||!std::getline(input,sha)||!std::getline(input,size)||!std::getline(input,original)||
       std::getline(input,extra)||magic!="INFINITY-CONTENT-BACKUP-1"||sha.size()!=64||
       sha.find_first_not_of("0123456789abcdef")!=std::string::npos||size.empty()||size.size()>9||
       size.find_first_not_of("0123456789")!=std::string::npos||original.empty())return false;
    unsigned long long count=0;for(char c:size)count=count*10+c-'0';
    if(count<0x1711||count>512ULL*1024*1024)return false;
    std::ifstream payload(file.c_str(),std::ios::binary|std::ios::ate);
    if(!payload||payload.tellg()<0||static_cast<unsigned long long>(payload.tellg())!=count)return false;
    XboxContentPackage p{};
    if(!ContentCatalog::ReadHeader(file,p)||p.titleId!=title||p.volumeType!=0||
       (p.contentType!=1&&p.contentType!=2&&p.contentType!=0xb0000))return false;
    char type[9];std::sprintf(type,"%08X",p.contentType);if(slot.substr(0,8)!=type)return false;
    out.package=p;out.package.name=slot+" | "+p.name;out.slot=slot;out.sha256=sha;out.original=original;out.bytes=count;return true;
}
}
bool ReadRecord(const FileWorkspace& ws,const std::string& root,unsigned title,const std::string& slot,Record& out){
    out={};return record(ws,root,title,slot,out);
}
std::vector<Record> List(const FileWorkspace& ws,const std::string& root,unsigned title){
    std::vector<Record> result;std::vector<FolderEntry> rows;
    const auto dir=titleFolder(root,"ContentBackups",title);
    if(!title||!ws.CanBrowse(dir)||!ReadFolder(dir,rows))return result;
    for(const auto& entry:rows){
        if(result.size()>=96)break;
        Record r;if(entry.directory&&record(ws,root,title,entry.name,r))result.push_back(r);
    }
    return result;
}
bool Recover(const FileWorkspace& ws,const std::string& root,const Record& selected,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error){
    error.clear();
#if defined(_WIN32)||defined(_XBOX)
    (void)ws;(void)root;(void)selected;(void)progress;error="Provider indisponivel.";return false;
#else
    auto reject=[&](const char* msg){error=msg;return false;};
    Record r;
    if(!enabled(root)||!record(ws,root,selected.package.titleId,selected.slot,r)||
       !same(r.package,selected.package)||r.sha256!=selected.sha256||r.bytes!=selected.bytes)return reject("Backup mudou ou recuperacao bloqueada.");
    const auto personal=FileWorkspace::Join(root,"UserFiles");
    const bool tu=r.package.contentType==0xb0000;
    const char* area=tu?"TitleUpdates":"RecoveredContent";
    const auto parent=FileWorkspace::Join(personal,area),folder=titleFolder(root,area,r.package.titleId);
    const auto capacity=StorageInfo::Read(personal);
    if(!ws.CanWrite(personal)||!capacity.available||capacity.writableBytes<r.bytes+16ULL*1024*1024||
       (!ws.CanWrite(parent)&&!ws.Create(personal,area))||
       (!ws.CanWrite(folder)&&!ws.Create(parent,folder.substr(folder.find_last_of("/\\")+1))))return reject("Destino pessoal ou espaco indisponivel.");
    const auto dest=FileWorkspace::Join(folder,"backup-"+r.slot+(tu?".tu":".bin")),temp=FileWorkspace::Join(folder,"AXCOPY-RECOVER.part");
    if(ws.CanReadFile(dest))return reject("Copia recuperada ja existe.");
    int flags=O_WRONLY|O_CREAT|O_EXCL;
#ifndef __XSDK__
    flags|=O_NOFOLLOW;
#endif
    int fd=open(temp.c_str(),flags,0600);if(fd<0)return reject("Temporario indisponivel.");
    auto fail=[&](const char* msg){if(fd>=0)close(fd);unlink(temp.c_str());error=msg;return false;};
    std::ifstream input(r.package.path.c_str(),std::ios::binary);
    StoreSha256 sha;unsigned char buf[32768];unsigned long long done=0;
    if(!progress(0,r.bytes*2))return fail("Cancelado.");
    while(done<r.bytes){
        const size_t n=static_cast<size_t>(std::min<unsigned long long>(sizeof buf,r.bytes-done));
        if(!input.read(reinterpret_cast<char*>(buf),n))return fail("Backup ilegivel.");
        size_t pos=0;while(pos<n){const ssize_t wrote=write(fd,buf+pos,n-pos);if(wrote<0&&errno==EINTR)continue;if(wrote<=0)return fail("Falha escrita.");pos+=static_cast<size_t>(wrote);}
        sha.Update(buf,n);done+=n;if(!progress(done,r.bytes*2))return fail("Cancelado.");
    }
    if(input.peek()!=std::char_traits<char>::eof()||sha.Finish()!=r.sha256)return fail("SHA-256 do backup divergente.");
    const bool synced=fsync(fd)==0,closed=close(fd)==0;fd=-1;
    if(!synced||!closed)return fail("Falha ao sincronizar.");
    StoreSha256 check;std::ifstream readback(temp.c_str(),std::ios::binary);
    for(unsigned long long pos=0;pos<r.bytes;){
        const size_t n=static_cast<size_t>(std::min<unsigned long long>(sizeof buf,r.bytes-pos));
        if(!readback.read(reinterpret_cast<char*>(buf),n))return fail("Falha releitura.");
        check.Update(buf,n);pos+=n;if(!progress(r.bytes+pos,r.bytes*2))return fail("Cancelado.");
    }
    if(check.Finish()!=r.sha256||readback.peek()!=std::char_traits<char>::eof())return fail("Copia divergente.");
    readback.close();XboxContentPackage verify{};
    if(!ContentCatalog::ReadHeader(temp,verify)||!same(verify,r.package))return fail("Cabecalho divergente.");
#ifdef __XSDK__
    if(!ws.PublishStaged(temp,dest))return fail("Falha ao publicar sem sobrescrita.");
#else
    if(link(temp.c_str(),dest.c_str())!=0)return fail("Falha ao publicar sem sobrescrita.");
    unlink(temp.c_str());
#endif
    return true;
#endif
}
bool ArchiveStaged(const FileWorkspace& ws,const std::string& root,const XboxContentPackage& selected,std::string& error){
    error.clear();if(!enabled(root)){error="Habilite backup de conteudo.";return false;}
    const auto list=Staged(ws,root,selected.titleId);
    const auto it=std::find_if(list.begin(),list.end(),[&](const XboxContentPackage& p){return p.path==selected.path&&same(p,selected);});
    if(it==list.end()){error="TU baixada mudou.";return false;}
    const auto personal=FileWorkspace::Join(root,"UserFiles"),parent=FileWorkspace::Join(personal,"ArchivedTU"),folder=titleFolder(root,"ArchivedTU",selected.titleId);
    if(!ws.CanWrite(personal)||(!ws.CanWrite(parent)&&!ws.Create(personal,"ArchivedTU"))||
       (!ws.CanWrite(folder)&&!ws.Create(parent,folder.substr(folder.find_last_of("/\\")+1)))){error="Pasta arquivo indisponivel.";return false;}
    const auto receipt=it->path+".receipt",savedReceipt=FileWorkspace::Join(folder,it->name+".receipt");
    const bool hasReceipt=ws.CanReadFile(receipt);
    if(hasReceipt&&!ws.MovePersonalExclusive(receipt,savedReceipt,false)){error="Recibo nao arquivado; TU preservada.";return false;}
    if(!ws.MovePersonalExclusive(it->path,FileWorkspace::Join(folder,it->name),false)){
        if(hasReceipt&&!ws.MovePersonalExclusive(savedReceipt,receipt,false))error="TU preservada; recibo em ArchivedTU.";
        else error="Conflito ou falha; origem preservada.";return false;
    }
    return true;
}

static std::vector<XboxContentPackage> ListTU(const FileWorkspace& ws,const std::string& root,unsigned title,const char* area){
    std::vector<XboxContentPackage> out;
    if(!title)return out;
    const auto folder=titleFolder(root,area,title);
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
std::vector<XboxContentPackage> Staged(const FileWorkspace& ws,const std::string& root,unsigned title){
    return ListTU(ws,root,title,"TitleUpdates");
}
std::vector<XboxContentPackage> Archived(const FileWorkspace& ws,const std::string& root,unsigned title){
    return ListTU(ws,root,title,"ArchivedTU");
}
bool RestoreArchived(const FileWorkspace& ws,const std::string& root,const XboxContentPackage& selected,std::string& error){
    error.clear();if(!enabled(root)){error="Habilite backup de conteudo.";return false;}
    const auto list=Archived(ws,root,selected.titleId);
    const auto it=std::find_if(list.begin(),list.end(),[&](const XboxContentPackage& p){return p.path==selected.path&&same(p,selected);});
    if(it==list.end()){error="TU arquivada mudou.";return false;}
    const auto personal=FileWorkspace::Join(root,"UserFiles"),parent=FileWorkspace::Join(personal,"TitleUpdates"),folder=titleFolder(root,"TitleUpdates",selected.titleId);
    if(!ws.CanWrite(personal)||(!ws.CanWrite(parent)&&!ws.Create(personal,"TitleUpdates"))||
       (!ws.CanWrite(folder)&&!ws.Create(parent,folder.substr(folder.find_last_of("/\\")+1)))){error="Pasta TU indisponivel.";return false;}
    const auto dest=FileWorkspace::Join(folder,it->name),receipt=it->path+".receipt",destReceipt=dest+".receipt";
    if(ws.CanReadFile(dest)||ws.CanReadFile(destReceipt)){error="Destino existente; nenhuma sobrescrita.";return false;}
    const bool hasReceipt=ws.CanReadFile(receipt);
    if(hasReceipt&&!ws.MovePersonalExclusive(receipt,destReceipt,false)){error="Recibo nao recuperado; TU preservada.";return false;}
    if(!ws.MovePersonalExclusive(it->path,dest,false)){
        if(hasReceipt&&!ws.MovePersonalExclusive(destReceipt,receipt,false))error="TU preservada; recibo em TitleUpdates.";
        else error="Conflito ou falha; origem preservada.";
        return false;
    }
    return true;
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
