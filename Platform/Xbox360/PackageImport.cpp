#include "PackageImport.h"
#include "StorageInfo.h"
#include "StoreSha256.h"
#include "GameScanner.h"
#include <fstream>
#include <cstdio>
#include <algorithm>
#if !defined(_XBOX)&&!defined(_WIN32)
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
namespace fs=std::filesystem;
#endif
namespace PackageImport {
namespace {
std::string area(const std::string& root,const char* name){return FileWorkspace::Join(FileWorkspace::Join(root,"UserFiles"),name);}
bool slotName(const std::string& name){return name.size()==16&&name.find_first_not_of("0123456789abcdef")==std::string::npos;}
bool enabled(const std::string& root){
    std::ifstream file(FileWorkspace::Join(root,"enable-package-import.txt").c_str());std::string line;
    return bool(std::getline(file,line))&&line=="ENABLE-PACKAGE-IMPORT";
}
}
std::vector<std::string> Roots(const std::string& root){
    std::vector<FolderEntry> rows;std::vector<std::string> result;
    const auto parent=area(root,"PackageLibrary");
    if(!ReadFolder(parent,rows))return result;
    for(const auto& row:rows){
        if(result.size()>=256)break;
        if(row.directory&&slotName(row.name))result.push_back(FileWorkspace::Join(parent,row.name));
    }
    return result;
}
bool Read(const FileWorkspace& ws,const std::string& root,PackageLaunch::Ticket& ticket,std::string& error){
    ticket={};error.clear();auto reject=[&](const char* msg){error=msg;return false;};
    const auto manifest=FileWorkspace::Join(root,"infinity360-package-import.txt");
    if(!ws.CanReadFile(manifest))return reject("Manifesto de importacao ausente.");
    std::ifstream file(manifest.c_str(),std::ios::binary|std::ios::ate);
    if(!file||file.tellg()<1||file.tellg()>64)return reject("Manifesto invalido.");
    file.seekg(0);std::string name,extra;
    if(!std::getline(file,name)||std::getline(file,extra))return reject("Informe apenas o nome do pacote.");
    if(!name.empty()&&name.back()=='\r')name.pop_back();
    if(name.empty()||name.size()>37||name.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos||!FileWorkspace::ValidName(name))return reject("Nome original hexadecimal invalido.");
    const auto path=FileWorkspace::Join(area(root,"PackageImport"),name);
    if(!ws.CanReadPersonalFile(path)||!PackageLaunch::Capture(path,ticket))return reject("GOD/XBLA incompleto ou formato nao suportado.");
    return true;
}
bool Install(const FileWorkspace& ws,const std::string& root,const PackageLaunch::Ticket& expected,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error){
    error.clear();
#if defined(_XBOX)||defined(_WIN32)
    (void)ws;(void)root;(void)expected;(void)progress;error="Provider indisponivel.";return false;
#else
    auto reject=[&](const char* msg){error=msg;return false;};
    PackageLaunch::Ticket current;
    if(!enabled(root)||!ws.CanWrite(area(root,"PackageImport")))return reject("Habilite escrita e enable-package-import.txt.");
    if(!Read(ws,root,current,error))return false;
    if(!PackageLaunch::Same(expected,current))return reject("Pacote mudou; confirme novamente.");
    if(Roots(root).size()>=256)return reject("Limite de 256 pacotes gerenciados.");
    const auto source=current.package.path,name=source.substr(source.find_last_of("/\\")+1);
    struct Part{std::string relative;unsigned long long bytes;};std::vector<Part> parts;
    unsigned long long total=0;
    auto add=[&](const std::string& relative){
        const auto path=FileWorkspace::Join(area(root,"PackageImport"),relative);
        std::error_code ec;const auto status=fs::symlink_status(path,ec);
        if(ec||!fs::is_regular_file(status)||!ws.CanReadPersonalFile(path))return false;
        const auto size=fs::file_size(path,ec);
        if(ec||!size||size>0xffffffffULL||size>16ULL*1024*1024*1024-total)return false;
        total+=size;parts.push_back({relative,size});return true;
    };
    if(!add(name))return reject("Cabecalho ilegivel.");
    for(unsigned i=0;i<current.package.dataFileCount;++i){
        char fragment[16];std::sprintf(fragment,"Data%04u",i);
        if(!progress(0,1)||!add(name+".data/"+fragment))return reject("Fragmento invalido ou cancelado.");
    }
    const auto capacity=StorageInfo::Read(area(root,"PackageImport"));
    if(!capacity.available||capacity.writableBytes<total+16ULL*1024*1024+parts.size()*65536ULL)return reject("Espaco insuficiente.");
    const auto parent=area(root,"PackageLibrary");
    if(!ws.CanWrite(parent)&&!ws.Create(FileWorkspace::Join(root,"UserFiles"),"PackageLibrary"))return reject("Biblioteca de pacotes indisponivel.");
    char slot[17],title[9],type[9];
    std::sprintf(slot,"%016llx",current.fingerprint);std::sprintf(title,"%08X",current.package.titleId);std::sprintf(type,"%08X",current.package.contentType);
    std::string stage;
    if(!ws.BeginDiscStage(parent,slot,stage))return reject("Pacote ja instalado ou pasta indisponivel.");
    std::vector<std::string> made;
    auto fail=[&](const char* msg){for(auto i=made.rbegin();i!=made.rend();++i){std::error_code ec;fs::remove(*i,ec);}std::error_code ec;fs::remove(stage,ec);error=msg;return false;};
    auto make=[&](const std::string& path){std::error_code ec;if(!fs::create_directory(path,ec)||ec)return false;made.push_back(path);return true;};
    const auto titleDir=FileWorkspace::Join(stage,title),target=FileWorkspace::Join(titleDir,type);
    if(!make(titleDir)||!make(target)||(current.package.volumeType==1&&!make(FileWorkspace::Join(target,name+".data"))))return fail("Falha ao criar estrutura.");
    unsigned char buffer[32768];unsigned long long done=0;
    for(const auto& part:parts){
        if(!progress(done,total*2))return fail("Cancelado.");
        const auto from=FileWorkspace::Join(area(root,"PackageImport"),part.relative),to=FileWorkspace::Join(target,part.relative);
        if(from.size()>240||to.size()>240)return fail("Caminho extenso.");
        std::ifstream in(from.c_str(),std::ios::binary|std::ios::ate);
        if(!in||in.tellg()<0||static_cast<unsigned long long>(in.tellg())!=part.bytes)return fail("Origem alterada.");
        in.seekg(0);int flags=O_WRONLY|O_CREAT|O_EXCL;
#ifndef __XSDK__
        flags|=O_NOFOLLOW;
#endif
        int fd=open(to.c_str(),flags,0600);if(fd<0)return fail("Falha ao criar arquivo.");
        made.push_back(to);StoreSha256 digest,check;
        for(unsigned long long pos=0;pos<part.bytes;){
            const size_t size=static_cast<size_t>(std::min<unsigned long long>(sizeof buffer,part.bytes-pos));
            if(!in.read(reinterpret_cast<char*>(buffer),size)){close(fd);return fail("Erro de leitura.");}
            size_t at=0;while(at<size){const ssize_t count=write(fd,buffer+at,size-at);if(count<0&&errno==EINTR)continue;if(count<=0){close(fd);return fail("Erro de escrita.");}at+=static_cast<size_t>(count);}
            digest.Update(buffer,size);pos+=size;done+=size;if(!progress(done,total*2)){close(fd);return fail("Cancelado.");}
        }
        const bool complete=in.peek()==std::char_traits<char>::eof()&&!in.bad();
        const bool synced=fsync(fd)==0,closed=close(fd)==0;if(!complete||!synced||!closed)return fail("Origem alterada ou falha de sincronizacao.");
        std::ifstream out(to.c_str(),std::ios::binary);
        for(unsigned long long pos=0;pos<part.bytes;){
            const size_t size=static_cast<size_t>(std::min<unsigned long long>(sizeof buffer,part.bytes-pos));
            if(!out.read(reinterpret_cast<char*>(buffer),size))return fail("Falha de releitura.");
            check.Update(buffer,size);pos+=size;done+=size;if(!progress(done,total*2))return fail("Cancelado.");
        }
        if(out.peek()!=std::char_traits<char>::eof()||out.bad()||check.Finish()!=digest.Finish())return fail("SHA-256 divergente.");
    }
    PackageLaunch::Ticket last{},copied{};
    if(!enabled(root)||!Read(ws,root,last,error)||!PackageLaunch::Same(expected,last)||
       !PackageLaunch::Capture(FileWorkspace::Join(target,name),copied)||copied.fingerprint!=current.fingerprint||copied.bytes!=current.bytes)return fail("Estrutura final divergente.");
    if(!ws.PublishDiscStage(parent,stage,slot))return fail("Falha na publicacao exclusiva.");
    return true;
#endif
}
}
