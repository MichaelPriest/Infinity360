#include "DiscInstaller.h"
#include "StorePackage.h"
#include "StorageInfo.h"
#include "ContentCatalog.h"
#include "GameScanner.h"
#include "XexLaunchCheck.h"
#include <cstdio>
#include <cctype>
#if !defined(_XBOX) && !defined(_WIN32)
#include <filesystem>
#include <set>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
namespace fs=std::filesystem;
namespace {
struct Entry {std::string relative;bool directory;unsigned long long bytes;};
std::string lower(std::string value){for(char& ch:value)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));return value;}
bool plan(const FileWorkspace& ws,const std::string& source,const std::string& relative,
          unsigned depth,std::vector<Entry>& entries,std::set<std::string>& names,unsigned long long& total){
    if(depth>16)return false;
    const std::string folder=relative.empty()?source:FileWorkspace::Join(source,relative);
    if(!ws.CanBrowse(folder))return false;
    std::vector<FolderEntry> children;
    if(!ReadFolder(folder,children))return false;
    for(const auto& child:children){
        if(child.name=="."||child.name=="..")continue;
        if(entries.size()>=16384||!FileWorkspace::ValidName(child.name))return false;
        const std::string rel=relative.empty()?child.name:relative+"/"+child.name;
        if(rel.size()>140||!names.insert(lower(rel)).second)return false;
        const std::string path=FileWorkspace::Join(source,rel);
        std::error_code ec;
        const auto st=fs::symlink_status(path,ec);
        if(ec||fs::is_symlink(st))return false;
        if(child.directory){
            if(!fs::is_directory(st))return false;
            entries.push_back({rel,true,0});
            if(!plan(ws,source,rel,depth+1,entries,names,total))return false;
        }else{
            if(!fs::is_regular_file(st))return false;
            auto bytes=fs::file_size(path,ec);
            if(ec||bytes>0xffffffffULL||bytes>16ULL*1024*1024*1024-total)return false;
            total+=bytes;entries.push_back({rel,false,bytes});
        }
    }
    return true;
}
bool execution(const std::string& path,XboxExecutionInfo& info){
    std::ifstream input(path.c_str(),std::ios::binary);
    return XexLaunchCheck::Header(input)&&ContentCatalog::ReadExecutionInfo(path,info);
}
}
#endif
bool InstallDiscFiles(const FileWorkspace& ws,const std::string& appRoot,
    const std::string& source,const std::string& mount,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,
    std::string& installedPath,std::string& error){
    installedPath.clear();error.clear();
    const auto reject=[&](const char* message){error=message;return false;};
#if defined(_XBOX) || defined(_WIN32)
    (void)ws;(void)appRoot;(void)source;(void)mount;(void)progress;
    return reject("Instalador DVD indisponivel neste provider.");
#else
#ifdef __XSDK__
    if(!GameScanner::SameRoot(source,"dvd:/")||
       (mount!="hdd:/"&&mount!="usb0:/"&&mount!="usb1:/"&&mount!="usb2:/"))return reject("Origem ou destino DVD invalido.");
#endif
    std::ifstream flag(FileWorkspace::Join(appRoot,"enable-dvd-install.txt").c_str());
    std::string marker;
    if(!std::getline(flag,marker)||marker!="ENABLE-DVD-INSTALL")return reject("Habilite enable-dvd-install.txt.");
    const std::string personal=ws.WorkspaceFor(mount);
    if(personal.empty()||!ws.CanBrowse(mount)||!ws.CanBrowse(source))return reject("DVD ou destino indisponivel; habilite area pessoal.");
    const std::string defaultXex=FileWorkspace::Join(source,"default.xex");
    XboxExecutionInfo info{};
    if(!execution(defaultXex,info))return reject("DVD sem default.xex XEX2 com identificacao legivel.");
    std::vector<Entry> entries;std::set<std::string> names;unsigned long long total=0;
    if(!progress(0,1))return reject("Instalacao cancelada.");
    if(!plan(ws,source,"",0,entries,names,total)||!total||!names.count("default.xex"))
        return reject("DVD ilegivel ou arvore/nome/tamanho incompativel com FATX.");
    const auto space=StorageInfo::Read(mount);
    // Reserve conservative metadata/allocation overhead in addition to bytes.
    if(!space.available||space.writableBytes<total+entries.size()*65536ULL+16ULL*1024*1024)
        return reject("Espaco livre insuficiente ou capacidade nao consultavel.");
    if(!ws.CanWrite(personal)&&!ws.Create(mount,personal.substr(personal.find_last_of("/\\")+1)))
        return reject("Nao foi possivel criar area pessoal.");
    const std::string parent=FileWorkspace::Join(personal,"Games");
    if(!ws.CanWrite(parent)&&!ws.Create(personal,"Games"))return reject("Pasta Games indisponivel.");
    char name[42];std::sprintf(name,"DVD-%08X-D%u-%08X",info.titleId,info.discNumber,info.mediaId);
    std::string stage;
    if(!ws.BeginDiscStage(parent,name,stage))return reject("Destino existente ou nao foi possivel reservar pasta.");
    std::vector<std::string> made;
    auto fail=[&](const char* reason){
        // Only remove objects created by this operation. Unknown content is never recursively deleted.
        for(auto it=made.rbegin();it!=made.rend();++it){std::error_code ec;fs::remove(*it,ec);}
        std::error_code ec;fs::remove(stage,ec);error=reason;return false;
    };
    unsigned long long done=0;
    unsigned char buffer[65536];
    for(const auto& e:entries){
        if(!progress(done,total*2))return fail("Instalacao cancelada.");
        const std::string from=FileWorkspace::Join(source,e.relative);
        const std::string to=FileWorkspace::Join(stage,e.relative);
        if(to.size()>240||from.size()>240)return fail("Caminho acima do limite.");
        if(e.directory){
            std::error_code ec;
            if(!fs::create_directory(to,ec)||ec)return fail("Falha ao criar pasta.");
            made.push_back(to);continue;
        }
        std::ifstream in(from.c_str(),std::ios::binary|std::ios::ate);
        if(!in||in.tellg()<0||static_cast<unsigned long long>(in.tellg())!=e.bytes)return fail("DVD removido ou arquivo alterado.");
        in.seekg(0);
        int flags=O_WRONLY|O_CREAT|O_EXCL;
#ifndef __XSDK__
        flags|=O_NOFOLLOW;
#endif
        int fd=open(to.c_str(),flags,0600);
        if(fd<0)return fail("Falha ao criar arquivo exclusivo.");
        made.push_back(to);
        StoreSha256 copied;
        unsigned long long pos=0;
        while(pos<e.bytes){
            const size_t n=static_cast<size_t>(e.bytes-pos>sizeof buffer?sizeof buffer:e.bytes-pos);
            if(!in.read(reinterpret_cast<char*>(buffer),n)){close(fd);return fail("Erro de leitura DVD.");}
            size_t at=0;
            while(at<n){
                ssize_t written=write(fd,buffer+at,n-at);
                if(written<0&&errno==EINTR)continue;
                if(written<=0){close(fd);return fail("Erro de escrita.");}
                at+=static_cast<size_t>(written);
            }
            copied.Update(buffer,n);pos+=n;done+=n;
            if(!progress(done,total*2)){close(fd);return fail("Instalacao cancelada.");}
        }
        if(in.peek()!=std::char_traits<char>::eof()||in.bad()){close(fd);return fail("DVD alterado durante copia.");}
        const bool flushed=fsync(fd)==0;const bool closed=close(fd)==0;
        if(!flushed||!closed)return fail("Falha ao sincronizar arquivo.");
        std::ifstream verify(to.c_str(),std::ios::binary);
        if(!verify)return fail("Falha ao reabrir copia.");
        StoreSha256 readback;pos=0;
        while(pos<e.bytes){
            const size_t n=static_cast<size_t>(e.bytes-pos>sizeof buffer?sizeof buffer:e.bytes-pos);
            if(!verify.read(reinterpret_cast<char*>(buffer),n))return fail("Falha ao verificar copia.");
            readback.Update(buffer,n);pos+=n;done+=n;
            if(!progress(done,total*2))return fail("Instalacao cancelada.");
        }
        if(verify.peek()!=std::char_traits<char>::eof()||verify.bad()||readback.Finish()!=copied.Finish())
            return fail("Copia divergente.");
    }
    XboxExecutionInfo current{},copy{};
    if(!execution(defaultXex,current)||!execution(FileWorkspace::Join(stage,"default.xex"),copy)||
       current.titleId!=info.titleId||current.mediaId!=info.mediaId||current.discNumber!=info.discNumber||
       copy.titleId!=info.titleId||copy.mediaId!=info.mediaId||copy.discNumber!=info.discNumber)
        return fail("Identificacao do DVD mudou ou copia invalida.");
    if(!ws.PublishDiscStage(parent,stage,name))return fail("Falha ao publicar pasta sem sobrescrever.");
    installedPath=FileWorkspace::Join(parent,name);return true;
#endif
}
