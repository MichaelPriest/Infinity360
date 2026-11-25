#include "ContentInstall.h"
#include "ContentBackup.h"
#include "StorageInfo.h"
#include "StoreSha256.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#if !defined(_XBOX)&&!defined(_WIN32)
#include <filesystem>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
namespace fs=std::filesystem;
#endif
namespace ContentInstall {
namespace {
bool enabled(const std::string& root){
    std::ifstream flag(FileWorkspace::Join(root,"enable-content-install.txt").c_str());std::string line;
    return bool(std::getline(flag,line))&&line=="ENABLE-CONTENT-INSTALL";
}
bool same(const XboxContentPackage& a,const XboxContentPackage& b){
    return a.titleId==b.titleId&&a.mediaId==b.mediaId&&a.version==b.version&&a.contentType==b.contentType&&a.volumeType==b.volumeType&&
        a.baseVersion==b.baseVersion&&a.discNumber==b.discNumber&&a.discCount==b.discCount&&
        a.sizeBytes==b.sizeBytes&&a.dataFileCount==b.dataFileCount;
}
bool compatible(const XboxExecutionInfo& game,const XboxContentPackage& pkg){
    if(!game.titleId||pkg.titleId!=game.titleId||pkg.volumeType!=0)return false;
    return pkg.contentType==2u||CompatibleUpdate(game,pkg);
}
}
bool Read(const FileWorkspace& ws,const std::string& root,const XboxExecutionInfo& game,Request& out,std::string& error){
    out={};error.clear();auto reject=[&](const char* msg){error=msg;return false;};
    const auto manifest=FileWorkspace::Join(root,"infinity360-content-import.tsv");
    if(!ws.CanReadFile(manifest))return reject("Manifesto local ausente.");
    std::ifstream file(manifest.c_str(),std::ios::binary|std::ios::ate);
    if(!file||file.tellg()<1||file.tellg()>256)return reject("Manifesto invalido.");
    file.seekg(0);std::string line,extra;
    if(!std::getline(file,line)||std::getline(file,extra))return reject("Use um pacote por manifesto.");
    if(!line.empty()&&line.back()=='\r')line.pop_back();
    if(line.empty()||line.back()=='\t'||line.find('\0')!=std::string::npos)return reject("Manifesto invalido.");
    std::istringstream fields(line);std::string type,name,size,sha;
    if(!std::getline(fields,type,'\t')||!std::getline(fields,name,'\t')||!std::getline(fields,size,'\t')||
       !std::getline(fields,sha,'\t')||std::getline(fields,extra,'\t'))return reject("Campos do manifesto invalidos.");
    const unsigned code=type=="TU"?0xb0000u:type=="DLC"?2u:0u;
    if(!Filename(code,name)||!FileWorkspace::ValidName(name)||sha.size()!=64||sha.find_first_not_of("0123456789abcdef")!=std::string::npos||
       size.empty()||size.size()>9||size.find_first_not_of("0123456789")!=std::string::npos)return reject("Nome original, tipo, tamanho ou SHA-256 invalido.");
    unsigned long long bytes=0;for(char c:size)bytes=bytes*10+c-'0';
    if(bytes<0x1711||bytes>512ULL*1024*1024)return reject("Pacote acima do limite ou incompleto.");
    const auto path=FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(root,"UserFiles"),"ContentImport"),name);
    if(!ws.CanReadFile(path))return reject("Pacote nao encontrado em UserFiles/ContentImport.");
    std::ifstream input(path.c_str(),std::ios::binary|std::ios::ate);
    if(!input||input.tellg()<0||static_cast<unsigned long long>(input.tellg())!=bytes)return reject("Tamanho do pacote divergente.");
    XboxContentPackage pkg{};
    if(!ContentCatalog::ReadHeader(path,pkg)||pkg.contentType!=code||!compatible(game,pkg))return reject("Title ID, Media ID, Base Version ou tipo incompativel.");
    out.package=pkg;out.filename=name;out.sha256=sha;out.bytes=bytes;return true;
}
bool ReadStaged(const FileWorkspace& ws,const std::string& root,const XboxExecutionInfo& game,const std::string& staged,Request& out,std::string& error){
    out={};error.clear();auto reject=[&](const char* message){error=message;return false;};
    char media[10],title[9];std::sprintf(media,"%08X-",game.mediaId);std::sprintf(title,"%08X",game.titleId);
    if(!game.titleId||!game.mediaId||!game.baseVersion||staged.size()<13||staged.size()>22||
       staged.compare(0,9,media)!=0||staged.substr(staged.size()-3)!=".tu"||
       staged.substr(9,staged.size()-12).find_first_not_of("0123456789")!=std::string::npos||
       !FileWorkspace::ValidName(staged))return reject("TU sem identificacao de download.");
    const auto folder=FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(root,"UserFiles"),"TitleUpdates"),title);
    const auto payload=FileWorkspace::Join(folder,staged),receipt=payload+".receipt";
    if(!ws.CanReadFile(payload)||!ws.CanReadFile(receipt))return reject("TU sem recibo; importe manualmente com nome original.");
    std::ifstream input(receipt.c_str(),std::ios::binary|std::ios::ate);
    if(!input||input.tellg()<1||input.tellg()>256)return reject("Recibo invalido.");
    input.seekg(0);std::string magic,base,name,bytes,sha,extra;
    if(!std::getline(input,magic)||!std::getline(input,base)||!std::getline(input,name)||!std::getline(input,bytes)||!std::getline(input,sha)||std::getline(input,extra)||
       magic!="INFINITY-TU-1"||base!=std::to_string(game.baseVersion)||!Filename(0xb0000,name)||!FileWorkspace::ValidName(name)||
       bytes.empty()||bytes.size()>9||bytes.find_first_not_of("0123456789")!=std::string::npos||
       sha.size()!=64||sha.find_first_not_of("0123456789abcdef")!=std::string::npos)return reject("Recibo, nome original ou Base Version incompativel.");
    unsigned long long size=0;for(char c:bytes)size=size*10+c-'0';
    std::ifstream file(payload.c_str(),std::ios::binary|std::ios::ate);
    if(size<0x1711||size>128ULL*1024*1024||!file||file.tellg()<0||static_cast<unsigned long long>(file.tellg())!=size)return reject("Tamanho da TU divergente.");
    XboxContentPackage package{};
    if(!ContentCatalog::ReadHeader(payload,package)||package.contentType!=0xb0000u||!compatible(game,package))return reject("TU incompativel.");
    out.package=package;out.filename=name;out.sha256=sha;out.bytes=size;out.staged=staged;return true;
}
bool ReadBackup(const FileWorkspace& ws,const std::string& root,const XboxExecutionInfo& game,const std::string& slot,Request& out,std::string& error){
    out={};error.clear();
    ContentBackup::Record record;
    if(!ContentBackup::ReadRecord(ws,root,game.titleId,slot,record)){error="Backup ou recibo invalido.";return false;}
    if(!compatible(game,record.package)){error="Backup nao e TU/DLC compativel; recupere apenas copia pessoal.";return false;}
    const auto pos=record.original.find_last_of("/\\");
    const auto filename=record.original.substr(pos==std::string::npos?0:pos+1);
    if(!Filename(record.package.contentType,filename)||!FileWorkspace::ValidName(filename)){
        error="Nome original nao identificado; recupere copia pessoal.";return false;
    }
    out.package=record.package;out.filename=filename;out.sha256=record.sha256;out.bytes=record.bytes;out.backupSlot=slot;
    return true;
}
bool Install(const FileWorkspace& ws,const std::string& root,const XboxExecutionInfo& game,const Request& selected,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error){
    error.clear();
#if defined(_WIN32)||defined(_XBOX)
    (void)ws;(void)root;(void)game;(void)selected;(void)progress;error="Provider indisponivel.";return false;
#else
    auto reject=[&](const char* msg){error=msg;return false;};
    Request current;
    if(!enabled(root)||!ws.CanWrite(FileWorkspace::Join(root,"UserFiles")))return reject("Habilite escrita e enable-content-install.txt.");
    if(!selected.backupSlot.empty()){
        if(!ReadBackup(ws,root,game,selected.backupSlot,current,error))return false;
    }else if(selected.staged.empty()? !Read(ws,root,game,current,error):!ReadStaged(ws,root,game,selected.staged,current,error))return false;
    if(current.filename!=selected.filename||current.sha256!=selected.sha256||current.bytes!=selected.bytes||
       current.package.path!=selected.package.path||current.staged!=selected.staged||current.backupSlot!=selected.backupSlot||!same(current.package,selected.package))return reject("Manifesto ou pacote mudou; confirme novamente.");
#ifdef __XSDK__
    const std::string common="hdd:/Content/0000000000000000",volume="hdd:/";
#else
    const auto common=FileWorkspace::Join(FileWorkspace::Join(root,"Content"),"0000000000000000"),volume=root;
#endif
    const bool legacy=current.package.contentType==0xb0000u&&LegacyFilename(current.filename);
    const auto cache=FileWorkspace::Join(volume,"Cache");
    const auto capacity=StorageInfo::Read(volume);
    if(!ws.CanBrowse(legacy?cache:common)||!capacity.available||capacity.writableBytes<current.bytes+16ULL*1024*1024)return reject("Pasta de destino ou espaco indisponivel.");
    if(!progress(0,current.bytes*2))return reject("Cancelado.");
    char title[9],type[9];std::sprintf(title,"%08X",game.titleId);std::sprintf(type,"%08X",current.package.contentType);
    auto folder=legacy?cache:common;
    if(!legacy)for(const char* part:{title,type}){
        folder=FileWorkspace::Join(folder,part);
        if(!ws.CanBrowse(folder)){
            std::error_code ec;
            if(!fs::create_directory(folder,ec)||ec||!ws.CanBrowse(folder))return reject("Pasta de conteudo indisponivel.");
        }
    }
    const auto dest=FileWorkspace::Join(folder,current.filename),temp=FileWorkspace::Join(folder,"AXCOPY-CONTENT.part");
    std::error_code ec;if(fs::exists(dest,ec)||ec)return reject("Conteudo existente; nenhuma sobrescrita.");
    int flags=O_WRONLY|O_CREAT|O_EXCL;
#ifndef __XSDK__
    flags|=O_NOFOLLOW;
#endif
    int fd=open(temp.c_str(),flags,0600);if(fd<0)return reject("Temporario existente ou indisponivel.");
    auto fail=[&](const char* msg){if(fd>=0)close(fd);unlink(temp.c_str());error=msg;return false;};
    std::ifstream input(current.package.path.c_str(),std::ios::binary);
    StoreSha256 sha;unsigned char buf[32768];unsigned long long done=0;
    while(done<current.bytes){
        const size_t n=static_cast<size_t>(std::min<unsigned long long>(sizeof buf,current.bytes-done));
        if(!input.read(reinterpret_cast<char*>(buf),n))return fail("Origem ilegivel.");
        size_t pos=0;while(pos<n){const ssize_t count=write(fd,buf+pos,n-pos);if(count<0&&errno==EINTR)continue;if(count<=0)return fail("Falha de escrita.");pos+=static_cast<size_t>(count);}
        sha.Update(buf,n);done+=n;if(!progress(done,current.bytes*2))return fail("Cancelado.");
    }
    if(input.peek()!=std::char_traits<char>::eof()||input.bad()||sha.Finish()!=current.sha256)return fail("SHA-256 divergente.");
    const bool synced=fsync(fd)==0,closed=close(fd)==0;fd=-1;
    if(!synced||!closed)return fail("Falha ao sincronizar.");
    StoreSha256 check;std::ifstream copied(temp.c_str(),std::ios::binary);
    for(unsigned long long pos=0;pos<current.bytes;){
        const size_t n=static_cast<size_t>(std::min<unsigned long long>(sizeof buf,current.bytes-pos));
        if(!copied.read(reinterpret_cast<char*>(buf),n))return fail("Falha ao reler.");
        check.Update(buf,n);pos+=n;if(!progress(current.bytes+pos,current.bytes*2))return fail("Cancelado.");
    }
    if(copied.peek()!=std::char_traits<char>::eof()||copied.bad()||check.Finish()!=current.sha256)return fail("Copia divergente.");
    copied.close();XboxContentPackage verified{};
    if(!enabled(root)||!ContentCatalog::ReadHeader(temp,verified)||!same(verified,current.package))return fail("Cabecalho ou permissao mudou.");
#ifdef __XSDK__
    if(!ws.PublishCommonContent(temp,dest,game.titleId,current.package.contentType))return fail("Falha ao publicar sem sobrescrever.");
#else
    if(link(temp.c_str(),dest.c_str())!=0)return fail("Falha ao publicar sem sobrescrever.");
    unlink(temp.c_str());
#endif
    return true;
#endif
}
}
