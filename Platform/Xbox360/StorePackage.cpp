#include "StorePackage.h"
#include "StorePartial.h"
#include "StoreInstallPath.h"
#include "XexLaunchCheck.h"
#include "MediaUrl.h"
#include "SavedFile.h"
#include <cstring>
#include <cstdio>
#include <fstream>
#include <sstream>
#if defined(_XBOX)
#include <xtl.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <sys/stat.h>
#endif
namespace {
std::string lower(std::string s){for(size_t i=0;i<s.size();++i)if(s[i]>='A'&&s[i]<='Z')s[i]+='a'-'A';return s;}
}
bool StoreNativeSource(const StoreItem& item,const std::string& root) {
    MediaUrl url;
    if(!url.Parse(item.url)||!url.secure||url.port!=443||item.url.find('#')!=std::string::npos)return false;
    // Explicit local allowlist of exact DNS names, no wildcard.
    std::ifstream in((root+"infinity360-store-hosts.txt").c_str(),std::ios::binary);
    char bytes[8193];in.read(bytes,sizeof bytes);
    if(in.bad()||in.gcount()>=static_cast<std::streamsize>(sizeof bytes))return false;
    std::istringstream lines(std::string(bytes,static_cast<size_t>(in.gcount())));
    std::string line;unsigned count=0;bool match=false;
    while(std::getline(lines,line)) {
        if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
        if(line.empty()||line[0]=='#')continue;
        if(++count>32||line.size()>253||line.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-")!=std::string::npos)return false;
        if(lower(line)==lower(url.host))match=true;
    }
    return match;
}
StorePackage::StorePackage(const FileWorkspace& workspace):workspace_(workspace),done_(0),active_(false),resumable_(false)
#if defined(_XBOX) || defined(_WIN32)
    ,handle_(INVALID_HANDLE_VALUE)
#else
    ,handle_(-1)
#endif
{}
StorePackage::~StorePackage(){Cancel();}
void StorePackage::Cancel() {
#if defined(_XBOX) || defined(_WIN32)
    if(handle_!=INVALID_HANDLE_VALUE){CloseHandle(handle_);handle_=INVALID_HANDLE_VALUE;}
    if(!temp_.empty())DeleteFileA(temp_.c_str());
#else
    if(handle_>=0){close(handle_);handle_=-1;}
    if(!temp_.empty())unlink(temp_.c_str());
#endif
    temp_.clear();active_=false;resumable_=false;
}
bool StorePackage::Pause(){
#if defined(_XBOX) || defined(_WIN32)
    return false;
#else
    if(!active_||!resumable_||handle_<0)return false;
    const bool flushed=fsync(handle_)==0;
    const bool closed=close(handle_)==0;handle_=-1;
    if(!flushed||!closed)return Fail("Falha ao preservar parcial.");
    temp_.clear();active_=false;resumable_=false;return true;
#endif
}
bool StorePackage::Fail(const char* message){Cancel();error_=message;return false;}
bool StorePackage::Begin(const StoreItem& item,const std::string& root,bool resume) {
    Cancel();error_.clear();done_=0;hash_=StoreSha256();item_=item;
    if(item.id.empty()||item.id.size()>32||item.id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-")!=std::string::npos||
       item.sha256.size()!=64||item.sha256.find_first_not_of("0123456789abcdef")!=std::string::npos||
       !item.bytes||item.bytes>512ULL*1024*1024)return Fail("Pacote invalido ou acima de 512 MiB.");
    const std::string personal=FileWorkspace::Join(root,"UserFiles");
    folder_=FileWorkspace::Join(personal,"Store");
    if(!workspace_.CanWrite(personal)&&!workspace_.Create(root,"UserFiles"))return Fail("UserFiles indisponivel.");
    if(!workspace_.CanWrite(folder_)&&!workspace_.Create(personal,"Store"))return Fail("Pasta Store indisponivel.");
    if(!workspace_.CanWrite(folder_))return Fail("Destino invalido.");
    target_=FileWorkspace::Join(folder_,item.id+".pkg");
    // Final publication is exclusive even if another process creates the target.
#if defined(_XBOX) || defined(_WIN32)
    if(GetFileAttributesA(target_.c_str())!=INVALID_FILE_ATTRIBUTES)return Fail("Pacote local ja existe.");
#else
    if(access(target_.c_str(),F_OK)==0)return Fail("Pacote local ja existe.");
#endif
    if(resume){
#if defined(_XBOX) || defined(_WIN32)
        return Fail("Retomada persistente indisponivel neste provider.");
#else
        const std::string path=StorePartial::Path(folder_,item);
        if(path.empty())return Fail("Identificador parcial invalido.");
        int flags=O_RDWR;
#ifndef __XSDK__
        flags|=O_NOFOLLOW;
#endif
        handle_=open(path.c_str(),flags|O_CREAT|O_EXCL,0600);
        if(handle_<0){
            if(errno!=EEXIST||!workspace_.CanReadPersonalFile(path))return Fail("Parcial indisponivel.");
            handle_=open(path.c_str(),flags);
            if(handle_<0)return Fail("Nao foi possivel abrir parcial.");
        }
        temp_=path;
        struct stat info{};
        if(fstat(handle_,&info)!=0||!S_ISREG(info.st_mode)||info.st_size<0||
           static_cast<unsigned long long>(info.st_size)>item.bytes)return Fail("Parcial invalido descartado.");
        unsigned char buffer[32768];
        const unsigned long long bytes=static_cast<unsigned long long>(info.st_size);
        while(done_<bytes){
            const size_t want=static_cast<size_t>(bytes-done_>sizeof buffer?sizeof buffer:bytes-done_);
            ssize_t got=read(handle_,buffer,want);
            if(got<0&&errno==EINTR)continue;
            if(got<=0)return Fail("Falha ao reler parcial.");
            hash_.Update(buffer,static_cast<size_t>(got));done_+=static_cast<unsigned long long>(got);
        }
        if(lseek(handle_,0,SEEK_END)!=static_cast<off_t>(done_))return Fail("Parcial alterado.");
        resumable_=true;active_=true;return true;
#endif
    }
    for(unsigned i=0;i<32;++i) {
        char name[32];std::sprintf(name,"AXCOPY-STORE-%04u.part",i);
        const std::string candidate=FileWorkspace::Join(folder_,name);
#if defined(_XBOX) || defined(_WIN32)
        handle_=CreateFileA(candidate.c_str(),GENERIC_WRITE,0,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
        if(handle_==INVALID_HANDLE_VALUE)continue;
#else
#ifdef __XSDK__
        handle_=open(candidate.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
#else
        handle_=open(candidate.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
#endif
        if(handle_<0)continue;
#endif
        temp_=candidate;active_=true;return true;
    }
    return Fail("Nao foi possivel reservar arquivo temporario.");
}
bool StorePackage::Write(const unsigned char* bytes,unsigned count) {
    if(!active_||count>32768||count>item_.bytes-done_)return Fail("Tamanho recebido invalido.");
#if defined(_XBOX) || defined(_WIN32)
    DWORD written=0;if(!WriteFile(handle_,bytes,count,&written,0)||written!=count)return Fail("Falha de escrita.");
#else
    size_t at=0;
    while(at<count){ssize_t n=write(handle_,bytes+at,count-at);if(n<0&&errno==EINTR)continue;if(n<=0)return Fail("Falha de escrita.");at+=n;}
#endif
    const unsigned long long previous=done_;
    hash_.Update(bytes,count);done_+=count;
#if !defined(_XBOX) && !defined(_WIN32)
    if(resumable_&&previous/(1024*1024)!=done_/(1024*1024)&&fsync(handle_)!=0)return Fail("Falha ao gravar checkpoint.");
#else
    (void)previous;
#endif
    return true;
}
bool StorePackage::Finish() {
    if(!active_||done_!=item_.bytes||hash_.Finish()!=item_.sha256)return Fail("Tamanho ou SHA-256 divergente.");
    if(!workspace_.CanWrite(folder_))return Fail("Destino indisponivel.");
#if defined(_XBOX) || defined(_WIN32)
    bool flush=FlushFileBuffers(handle_)!=0;bool closed=CloseHandle(handle_)!=0;handle_=INVALID_HANDLE_VALUE;
    if(!flush||!closed||!MoveFileA(temp_.c_str(),target_.c_str()))return Fail("Falha ao publicar pacote.");
#else
    bool flush=fsync(handle_)==0;bool closed=close(handle_)==0;handle_=-1;
#ifdef __XSDK__
    if(!flush||!closed||!workspace_.PublishStaged(temp_,target_))return Fail("Falha ao publicar pacote sem sobrescrever.");
#else
    if(!flush||!closed||link(temp_.c_str(),target_.c_str())!=0)return Fail("Falha ao publicar pacote.");
    unlink(temp_.c_str());
#endif
#endif
    temp_.clear();active_=false;
    // Snapshot is informational, not a signature. Never replace the package.
    const std::string receipt=FileWorkspace::Join(folder_,item_.id+".state");
    std::ofstream out((receipt+".tmp").c_str(),std::ios::binary|std::ios::trunc);
    out<<item_.sha256<<"\tnative_verified\t"<<done_<<"\n";out.close();
    if(!out||!replaceSavedFile(receipt))error_="Pacote verificado; registro de estado indisponivel.";
    return true;
}

bool StoreInstallXex(const FileWorkspace& workspace,const StoreItem& item,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error){
    error.clear();
#if defined(_XBOX) || defined(_WIN32)
    (void)workspace;(void)item;(void)root;(void)progress;
    error="Instalacao nativa indisponivel neste provider.";return false;
#else
    const auto reject=[&](const char* reason){error=reason;return false;};
    std::ifstream opt(FileWorkspace::Join(root,"enable-store-install.txt").c_str());
    std::string signature;
    if(!std::getline(opt,signature)||signature!="ENABLE-STORE-INSTALL")return reject("Instalacao bloqueada: habilite enable-store-install.txt.");
    const std::string folder=StoreInstallPath::Folder(root,item);
    if(folder.empty()||item.bytes<4096||item.bytes>512ULL*1024*1024||
       item.sha256.size()!=64||item.sha256.find_first_not_of("0123456789abcdef")!=std::string::npos||item.rights.empty())
        return reject("Somente XEX independente de homebrew/app/emulador autorizado.");
    const std::string personal=FileWorkspace::Join(root,"UserFiles");
    const std::string store=FileWorkspace::Join(personal,"Store");
    const std::string source=FileWorkspace::Join(store,item.id+".pkg");
    if(!workspace.CanWrite(personal)||!workspace.CanReadPersonalFile(source))return reject("Pacote ou area pessoal indisponivel.");
    std::ifstream input(source.c_str(),std::ios::binary|std::ios::ate);
    if(!input||input.tellg()<0||static_cast<unsigned long long>(input.tellg())!=item.bytes||
       !XexLaunchCheck::Header(input))return reject("Pacote nao e um XEX2 independente valido.");
    input.clear();input.seekg(0,std::ios::beg);
    const std::string category=StoreInstallPath::Category(item);
    const std::string parent=FileWorkspace::Join(personal,category);
    if(!workspace.CanWrite(parent)&&!workspace.Create(personal,category))return reject("Pasta de categoria indisponivel.");
    // An existing folder is never merged, even if the prior attempt left files.
    if(!workspace.Create(parent,item.id))return reject("Destino ja existe ou indisponivel; nao sera sobrescrito.");
    const std::string target=FileWorkspace::Join(folder,"default.xex");
    const std::string temp=FileWorkspace::Join(folder,"AXCOPY-INSTALL.part");
    const std::string receipt=FileWorkspace::Join(folder,"infinity-install.txt");
    bool receiptCreated=false;
    int flags=O_WRONLY|O_CREAT|O_EXCL;
#ifndef __XSDK__
    flags|=O_NOFOLLOW;
#endif
    int fd=open(temp.c_str(),flags,0600);
    if(fd<0){workspace.Remove(parent,item.id);return reject("Falha ao reservar temporario.");}
    auto fail=[&](const char* reason){
        if(fd>=0){close(fd);fd=-1;}
        unlink(temp.c_str());if(receiptCreated)unlink(receipt.c_str());workspace.Remove(parent,item.id);error=reason;return false;
    };
    StoreSha256 hash;
    unsigned long long done=0;
    unsigned char block[32768];
    if(!progress(0,item.bytes))return fail("Instalacao cancelada.");
    while(done<item.bytes){
        const size_t n=static_cast<size_t>(item.bytes-done>sizeof block?sizeof block:item.bytes-done);
        if(!input.read(reinterpret_cast<char*>(block),static_cast<std::streamsize>(n)))return fail("Falha ao ler pacote.");
        size_t at=0;
        while(at<n){
            const ssize_t written=write(fd,block+at,n-at);
            if(written<0&&errno==EINTR)continue;
            if(written<=0)return fail("Falha ao gravar aplicativo.");
            at+=static_cast<size_t>(written);
        }
        hash.Update(block,n);done+=n;
        if(!progress(done,item.bytes))return fail("Instalacao cancelada.");
    }
    if(input.peek()!=std::char_traits<char>::eof()||hash.Finish()!=item.sha256)return fail("SHA-256 ou tamanho do pacote divergente.");
    const bool flushed=fsync(fd)==0;
    const bool closed=close(fd)==0;fd=-1;
    if(!flushed||!closed)return fail("Falha ao sincronizar aplicativo.");
    // Validate the staged executable as well, not only its original source.
    {std::ifstream staged(temp.c_str(),std::ios::binary);
     if(!XexLaunchCheck::Header(staged))return fail("XEX2 temporario invalido.");}
    {
        const int record=open(receipt.c_str(),flags,0600);
        if(record<0)return fail("Falha ao reservar registro.");
        receiptCreated=true;
        const std::string data=item.sha256+"\t"+std::to_string(item.bytes)+"\n";
        const bool saved=write(record,data.data(),data.size())==static_cast<ssize_t>(data.size());
        const bool synced=fsync(record)==0;const bool closed=close(record)==0;
        if(!saved||!synced||!closed)return fail("Falha ao gravar registro.");
    }
#ifdef __XSDK__
    if(!workspace.PublishStaged(temp,target))return fail("Falha ao publicar sem sobrescrever.");
#else
    if(!workspace.CanWrite(folder)||link(temp.c_str(),target.c_str())!=0)return fail("Falha ao publicar sem sobrescrever.");
    unlink(temp.c_str());
#endif
    return true;
#endif
}
