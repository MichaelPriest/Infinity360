#include "FileWorkspace.h"
#include <cctype>
#include <cstdio>
#include <fstream>
#include <cerrno>
#include <set>
#if defined(__linux__) && !defined(__XSDK__)
#include <sys/syscall.h>
#endif
#ifdef __XSDK__
#include "XsdkPaths.h"
#include "ContentInstall.h"
#include "xsdk/kernel.h"
#include <cstdlib>
#include <sys/stat.h>
#include "GameScanner.h"
#endif
#if defined(_WIN32) && !defined(_XBOX)
#include <windows.h>
#elif !defined(_XBOX)
#include <fcntl.h>
#include <unistd.h>
#endif
#ifdef _XBOX
#include <xtl.h>
#else
#include <filesystem>
namespace fs=std::filesystem;
#endif

namespace {
std::string key(std::string s) {
#ifdef __XSDK__
    const std::string normalized=XsdkPaths::Key(s);
    if(!normalized.empty())return normalized;
    for(char& ch:s)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
#elif defined(_XBOX)
    for(size_t i=0;i<s.size();++i) {
        if(s[i]=='/')s[i]='\\';
        s[i]=(char)std::tolower((unsigned char)s[i]);
    }
#endif
    while(s.size()>1 && (s[s.size()-1]=='/' || s[s.size()-1]=='\\'))
        s.erase(s.size()-1);
    return s;
}
bool inside(const std::string& path,const std::string& root) {
#ifdef __XSDK__
    return XsdkPaths::Inside(path,root);
#else
    const std::string p=key(path),r=key(root);
    if(p==r)return true;
    if(p.size()<=r.size() || p.compare(0,r.size(),r)!=0)return false;
    return r=="/" || p[r.size()]=='/' || p[r.size()]=='\\';
#endif
}
bool clean(const std::string& path) {
    if(path.empty() || path.size()>240)return false;
    size_t start=0;
    for(size_t i=0;i<=path.size();++i) {
        if(i<path.size()) {
            const unsigned char ch=(unsigned char)path[i];
            if(ch<32 || ch==127 || ch=='*' || ch=='?' || ch=='"' ||
               ch=='|' || ch=='<' || ch=='>')return false;
            if(ch!='/' && ch!='\\')continue;
        }
        const std::string part=path.substr(start,i-start);
        if(part=="." || part=="..")return false;
        start=i+1;
    }
#ifdef __XSDK__
    // FATX has no symbolic links; never permit internal drive changes or ..
    return !XsdkPaths::Normalize(path,true).empty();
#elif !defined(_XBOX)
    // Reject links, including links in parent components, even if the
    // target is inside the workspace. This also covers nonexistent targets.
    std::error_code ec;
    const fs::path absolute=fs::absolute(key(path),ec).lexically_normal();
    if(ec)return false;
    fs::path component;
    for(const auto& part:absolute) {
        component/=part;
        std::error_code linkError;
        if(fs::is_symlink(fs::symlink_status(component,linkError)))return false;
    }
    const fs::path canonical=fs::weakly_canonical(absolute,ec);
    if(ec || canonical!=absolute)return false;
#endif
    return true;
}
bool directory(const std::string& path) {
#ifdef _XBOX
    const DWORD attr=GetFileAttributesA(path.c_str());
    return attr!=INVALID_FILE_ATTRIBUTES && (attr & FILE_ATTRIBUTE_DIRECTORY)!=0;
#else
    std::error_code ec;
    return fs::is_directory(path,ec) && !ec;
#endif
}
bool exists(const std::string& path) {
#ifdef _XBOX
    return GetFileAttributesA(path.c_str())!=INVALID_FILE_ATTRIBUTES;
#else
    std::error_code ec;
    return fs::exists(path,ec) && !ec;
#endif
}
#ifdef __XSDK__
// XSDK POSIX rename overwrites and FATX has no hard links. Use the kernel
// no-replace flag instead; keep the original on collision or failure.
std::string kernelPath(const std::string& path){
    char* raw=realpath(path.c_str(),nullptr);
    if(!raw)return "";
    std::string result(raw);std::free(raw);
    for(char& ch:result)if(ch=='/')ch='\\';
    return result;
}
bool renameNoReplace(const std::string& from,const std::string& to){
    if(!clean(from)||!clean(to)||key(from)==key(to))return false;
    const std::string source=kernelPath(from);
    const size_t split=to.find_last_of("/\\");
    if(source.empty()||split==std::string::npos)return false;
    std::string target=kernelPath(to.substr(0,split+1));
    if(target.empty())return false;
    if(target.back()!='\\')target+='\\';
    target+=to.substr(split+1);
    if(target.size()>255||source.size()>255)return false;
    ANSI_STRING name{};name.length=static_cast<uint16_t>(source.size());
    name.maximum_length=name.length+1;name.pointer=source.c_str();
    OBJECT_ATTRIBUTES attr{};attr.root_directory=source.front()=='\\'?0:OB_DOS_DEVICES;attr.name=&name;
    HANDLE handle=0;IO_STATUS_BLOCK io{};
    NTSTATUS status=NtCreateFile(&handle,DELETE|SYNCHRONIZE,&attr,&io,nullptr,
        FILE_ATTRIBUTE_NORMAL,0,FILE_OPEN,FILE_SYNCHRONOUS_IO_NONALERT);
    if(!NT_SUCCESS(status))return false;
    FILE_RENAME_INFORMATION info{};info.replace_if_exists=0;
    info.name.length=static_cast<uint16_t>(target.size());
    info.name.maximum_length=info.name.length+1;info.name.pointer=target.c_str();
    status=NtSetInformationFile(handle,&io,&info,sizeof info,FILE_RENAME_INFORMATION_CLASS);
    const NTSTATUS closed=NtClose(handle);
    return NT_SUCCESS(status)&&NT_SUCCESS(closed);
}
#endif
}

FileWorkspace::FileWorkspace(const std::string& appDirectory) {
    roots_.push_back(appDirectory);
#ifdef __XSDK__
    const char* drives[]={"hdd:/","usb0:/","usb1:/","usb2:/","mu0:/","mu1:/","dvd:/"};
    roots_.insert(roots_.end(),drives,drives+7);
    std::ifstream marker(Join(appDirectory,"enable-file-writes.txt").c_str());
    std::string signature;
    if(std::getline(marker,signature)&&signature=="ENABLE-FILE-WRITES"){
        workspaces_.push_back(Join(appDirectory,"UserFiles"));
        for(size_t i=1;i<=4;++i)workspaces_.push_back(Join(roots_[i],"Infinity360Files"));
    }
#else
    workspaces_.push_back(Join(appDirectory,"UserFiles"));
#ifdef _XBOX
    roots_.push_back("HDD1:\\");
    roots_.push_back("USB0:\\");
    roots_.push_back("USB1:\\");
    for(size_t i=1;i<roots_.size();++i)
        workspaces_.push_back(Join(roots_[i],"AuroraXFiles"));
    // Optical disc and memory units are browse-only, not writable workspaces.
    roots_.push_back("DVD:\\");
    roots_.push_back("MU0:\\");
    roots_.push_back("MU1:\\");
#endif
#endif
}
bool FileWorkspace::ValidName(const std::string& name) {
    if(name.empty() || name.size()>42 || name=="." || name==".." ||
       name[0]==' ' || name[name.size()-1]==' ' || name[name.size()-1]=='.')
        return false;
    std::string upper=name;
    for(size_t i=0;i<upper.size();++i)upper[i]=(char)std::toupper((unsigned char)upper[i]);
    if(upper.compare(0,7,"AXCOPY-")==0)return false;
    for(size_t i=0;i<name.size();++i) {
        const unsigned char ch=(unsigned char)name[i];
        if(!(std::isalnum(ch) || ch==' ' || ch=='.' || ch=='-' || ch=='_'))
            return false;
    }
    return true;
}
std::string FileWorkspace::Join(const std::string& parent,const std::string& name) {
    if(parent.empty())return name;
    const char last=parent[parent.size()-1];
    if(last=='/' || last=='\\')return parent+name;
    return parent+(parent.find('\\')!=std::string::npos?"\\":"/")+name;
}
bool FileWorkspace::Allowed(const std::string& path,
                            const std::vector<std::string>& roots) const {
    if(!clean(path))return false;
    for(size_t i=0;i<roots.size();++i)
        if(inside(path,roots[i]))return true;
    return false;
}
bool FileWorkspace::CanBrowse(const std::string& path) const {
    return Allowed(path,roots_) && directory(path);
}
bool FileWorkspace::CanWrite(const std::string& path) const {
    return Allowed(path,workspaces_) && directory(path);
}
bool FileWorkspace::CanReadFile(const std::string& path) const {
    if(!Allowed(path,roots_)||!exists(path)||directory(path))return false;
#ifndef _XBOX
    std::error_code ec;return fs::is_regular_file(path,ec)&&!ec;
#else
    return true;
#endif
}
bool FileWorkspace::CanReadPersonalFile(const std::string& path) const {
    if(!Allowed(path,workspaces_) || !exists(path) || directory(path))return false;
#ifndef _XBOX
    std::error_code ec;
    return fs::is_regular_file(path,ec) && !ec;
#else
    return true;
#endif
}
#ifdef __XSDK__
bool FileWorkspace::MoveHddTitleUpdate(const std::string& source,const std::string& destination,unsigned title,bool restore) const{
    if(!title||roots_.empty()||workspaces_.empty())return false;
    std::ifstream marker(Join(roots_[0],"enable-content-install.txt").c_str());std::string enabled;
    if(!std::getline(marker,enabled)||enabled!="ENABLE-CONTENT-INSTALL")return false;
    const auto name=source.substr(source.find_last_of("/\\")+1);
    if(!ContentInstall::Filename(0xb0000u,name))return false;
    const bool legacy=ContentInstall::LegacyFilename(name);
    char id[9];std::sprintf(id,"%08X",title);
    const auto live=legacy?std::string("hdd:/Cache"):Join(Join("hdd:/Content/0000000000000000",id),"000B0000");
    const auto saved=Join(Join(Join("hdd:/Infinity360Files","DisabledTU"),id),legacy?"Cache":"Content");
    if(!CanWrite(saved)||!CanBrowse(live)||!CanReadFile(source)||exists(destination)||
       key(source)!=key(Join(restore?saved:live,name))||key(destination)!=key(Join(restore?live:saved,name)))return false;
    XboxContentPackage p{};
    if(!ContentCatalog::ReadHeader(source,p)||p.titleId!=title||p.contentType!=0xb0000u||p.volumeType!=0)return false;
    return renameNoReplace(source,destination);
}
bool FileWorkspace::PublishCommonContent(const std::string& temporary,const std::string& destination,unsigned title,unsigned type) const {
    if(!title||(type!=2u&&type!=0xb0000u)||workspaces_.empty()||roots_.empty())return false;
    std::ifstream marker(Join(roots_[0],"enable-content-install.txt").c_str());std::string enabled;
    if(!std::getline(marker,enabled)||enabled!="ENABLE-CONTENT-INSTALL")return false;
    char id[9],kind[9];std::sprintf(id,"%08X",title);std::sprintf(kind,"%08X",type);
    const auto name=destination.substr(destination.find_last_of("/\\")+1);
    const bool legacy=type==0xb0000u&&ContentInstall::LegacyFilename(name);
    const auto folder=legacy?std::string("hdd:/Cache"):Join(Join("hdd:/Content/0000000000000000",id),kind);
    if(!ContentInstall::Filename(type,name)||key(temporary)!=key(Join(folder,"AXCOPY-CONTENT.part"))||
       key(destination)!=key(Join(folder,name))||!CanReadFile(temporary)||!CanBrowse(folder)||exists(destination))return false;
    XboxContentPackage header{};
    if(!ContentCatalog::ReadHeader(temporary,header)||header.titleId!=title||header.contentType!=type||header.volumeType!=0)return false;
    return renameNoReplace(temporary,destination);
}
bool FileWorkspace::PublishStaged(const std::string& temporary,const std::string& destination) const {
    const size_t a=temporary.find_last_of("/\\"),b=destination.find_last_of("/\\");
    if(a==std::string::npos||b==std::string::npos)return false;
    const std::string name=temporary.substr(a+1);
    if(name.compare(0,7,"AXCOPY-")!=0||!ValidName(destination.substr(b+1)))return false;
    if(key(temporary.substr(0,a))!=key(destination.substr(0,b))||
       !CanWrite(destination.substr(0,b))||!CanReadPersonalFile(temporary)||
       !Allowed(destination,workspaces_)||exists(destination))return false;
    return renameNoReplace(temporary,destination);
}
#endif
std::string FileWorkspace::WorkspaceFor(const std::string& root) const {
    for(size_t i=0;i<roots_.size();++i)
        if(key(root)==key(roots_[i]))return i<workspaces_.size()?workspaces_[i]:"";
    return "";
}
bool FileWorkspace::MovePersonalExclusive(const std::string& from,const std::string& to,bool folder) const{
#if defined(__XSDK__) || defined(__linux__)
    const size_t a=from.find_last_of("/\\"),b=to.find_last_of("/\\");
    if(a==std::string::npos||b==std::string::npos||!ValidName(from.substr(a+1))||!ValidName(to.substr(b+1))||
       !CanWrite(from.substr(0,a))||!CanWrite(to.substr(0,b))||!Allowed(from,workspaces_)||!Allowed(to,workspaces_)||
       !exists(from)||exists(to)||inside(to,from)||inside(from,to)||directory(from)!=folder)return false;
    if(!folder&&!CanReadPersonalFile(from))return false;
#ifdef __XSDK__
    const auto x=XsdkPaths::Key(from),y=XsdkPaths::Key(to);
    if(x.substr(0,x.find(':'))!=y.substr(0,y.find(':')))return false;
    return renameNoReplace(from,to);
#else
#ifdef SYS_renameat2
    return syscall(SYS_renameat2,AT_FDCWD,from.c_str(),AT_FDCWD,to.c_str(),1)==0;
#endif
#endif
#else
    (void)from;(void)to;(void)folder;
#endif
    return false;
}
bool FileWorkspace::BeginDiscStage(const std::string& parent,const std::string& finalName,std::string& stage) const{
    stage.clear();
#if defined(__XSDK__) || defined(__linux__)
    if(!CanWrite(parent)||!ValidName(finalName)||exists(Join(parent,finalName)))return false;
    for(unsigned i=0;i<32;++i){
        char name[32];std::sprintf(name,"AXCOPY-DISC-%04u",i);
        const std::string candidate=Join(parent,name);
        if(!Allowed(candidate,workspaces_))return false;
        std::error_code ec;
        if(fs::create_directory(candidate,ec)&&!ec){stage=candidate;return true;}
    }
#else
    (void)parent;(void)finalName;
#endif
    return false;
}
bool FileWorkspace::PublishDiscStage(const std::string& parent,const std::string& stage,const std::string& finalName) const{
#if defined(__XSDK__) || defined(__linux__)
    const size_t at=stage.find_last_of("/\\");
    if(at==std::string::npos||key(stage.substr(0,at))!=key(parent)||
       stage.substr(at+1).compare(0,12,"AXCOPY-DISC-")!=0||!ValidName(finalName)||
       !CanWrite(parent)||!CanWrite(stage))return false;
    const std::string dest=Join(parent,finalName);
    if(!Allowed(dest,workspaces_)||exists(dest))return false;
#ifdef __XSDK__
    return renameNoReplace(stage,dest);
#else
#ifdef SYS_renameat2
    return syscall(SYS_renameat2,AT_FDCWD,stage.c_str(),AT_FDCWD,dest.c_str(),1)==0;
#endif
#endif
#else
    (void)parent;(void)stage;(void)finalName;
#endif
    return false;
}
bool FileWorkspace::Create(const std::string& parent,const std::string& name) const {
    if(!ValidName(name) || !CanBrowse(parent))return false;
    const std::string path=Join(parent,name);
    // At a mount root, creation is limited to its designated workspace.
    const std::string workspace=WorkspaceFor(parent);
    if(!CanWrite(parent) && (workspace.empty() || key(workspace)!=key(path)))return false;
    if(!Allowed(path,workspaces_) || exists(path))return false;
#ifdef _XBOX
    return CreateDirectoryA(path.c_str(),0)!=0;
#else
    std::error_code ec;
    return fs::create_directory(path,ec) && !ec;
#endif
}
bool FileWorkspace::Rename(const std::string& parent,const std::string& oldName,
                           const std::string& newName) const {
    if(!CanWrite(parent) || !ValidName(oldName) || !ValidName(newName))return false;
    const std::string from=Join(parent,oldName),to=Join(parent,newName);
    if(!Allowed(from,workspaces_) || !Allowed(to,workspaces_) ||
       !exists(from) || exists(to))return false;
#ifdef _XBOX
    return MoveFileA(from.c_str(),to.c_str())!=0;
#elif defined(__XSDK__)
    return renameNoReplace(from,to);
#else
    std::error_code ec;
    fs::rename(from,to,ec);
    return !ec;
#endif
}
bool FileWorkspace::Remove(const std::string& parent,const std::string& name) const {
    if(!CanWrite(parent)||!ValidName(name))return false;
    const std::string path=Join(parent,name);
    if(!Allowed(path,workspaces_)||!exists(path))return false;
    // Nonrecursive only: an empty directory or a regular personal file.
#ifdef __XSDK__
    if(directory(path)){
        std::vector<FolderEntry> children;
        if(!ReadFolder(path,children)||!children.empty())return false;
    }
#endif
#ifdef _XBOX
    return directory(path)?RemoveDirectoryA(path.c_str())!=0:DeleteFileA(path.c_str())!=0;
#else
    std::error_code ec;
    const fs::file_status st=fs::symlink_status(path,ec);
    if(ec||(!fs::is_directory(st)&&!fs::is_regular_file(st)))return false;
    return fs::remove(path,ec)&&!ec;
#endif
}
bool FileWorkspace::Move(const std::string& source,const std::string& name,const std::string& dest) const {
    if(!CanWrite(source)||!CanWrite(dest)||!ValidName(name))return false;
    const std::string from=Join(source,name),to=Join(dest,name);
    if(!Allowed(from,workspaces_)||!Allowed(to,workspaces_)||!exists(from)||exists(to)||
       inside(dest,from)||directory(from))return false;
#if defined(_XBOX) || defined(_WIN32)
    // MoveFile does not overwrite; reject moves across mount roots on Xbox.
#ifdef _XBOX
    if(key(source).substr(0,key(source).find(':'))!=key(dest).substr(0,key(dest).find(':')))return false;
#endif
    return MoveFileA(from.c_str(),to.c_str())!=0;
#else
#ifdef __XSDK__
    const std::string a=XsdkPaths::Key(source),b=XsdkPaths::Key(dest);
    if(a.substr(0,a.find(':'))!=b.substr(0,b.find(':')))return false;
    return renameNoReplace(from,to);
#else
    // A same-filesystem hard link reserves a new name without clobbering it.
    // If source unlink fails, leave both names rather than risk deleting data.
    std::error_code ec;
    if(!fs::is_regular_file(from,ec)||ec)return false;
    if(link(from.c_str(),to.c_str())!=0)return false;
    return unlink(from.c_str())==0;
#endif
#endif
}
bool FileWorkspace::PrepareCopy(const std::string& sourceDir,const std::string& name,
                                 const std::string& destDir,std::string& from,
                                 std::string& to) const {
    if(!CanBrowse(sourceDir) || !CanWrite(destDir) || !ValidName(name))return false;
    from=Join(sourceDir,name);to=Join(destDir,name);
    if(!Allowed(from,roots_) || !Allowed(to,workspaces_) || !exists(from) ||
       directory(from) || exists(to))return false;
#ifndef _XBOX
    std::error_code ec;
    if(!fs::is_regular_file(from,ec) || ec)return false;
#endif
    return true;
}
bool FileWorkspace::PlanCopies(const std::string& source,const std::vector<std::string>& names,
                               const std::string& destination,std::vector<WorkspaceCopyStep>& out) const {
    out.clear();
    if(!CanBrowse(source)||!CanWrite(destination)||names.empty()||names.size()>32)return false;
    std::vector<WorkspaceCopyStep> plan;
    std::set<std::string> unique;
    for(size_t i=0;i<names.size();++i) {
        if(!unique.insert(key(names[i])).second ||
           !PlanItem(source,names[i],destination,0,plan))return false;
    }
    out.swap(plan);return true;
}
bool FileWorkspace::PlanItem(const std::string& source,const std::string& name,
                             const std::string& destination,unsigned depth,
                             std::vector<WorkspaceCopyStep>& steps) const {
    if(depth>12||steps.size()>=256||!ValidName(name)||!CanBrowse(source))return false;
    const std::string from=Join(source,name),to=Join(destination,name);
    if(!Allowed(from,roots_)||!Allowed(to,workspaces_)||!exists(from)||exists(to))return false;
    const bool folder=directory(from);
    if(folder && inside(destination,from))return false;
#ifndef _XBOX
    std::error_code ec;
    const fs::file_status st=fs::symlink_status(from,ec);
    if(ec||(!fs::is_directory(st)&&!fs::is_regular_file(st)))return false;
#endif
    WorkspaceCopyStep step={source,name,destination,folder};steps.push_back(step);
    if(!folder)return true;
    std::vector<std::string> children;
#ifdef _XBOX
    WIN32_FIND_DATAA info;
    HANDLE handle=FindFirstFileA(Join(from,"*").c_str(),&info);
    if(handle==INVALID_HANDLE_VALUE)return GetLastError()==ERROR_FILE_NOT_FOUND;
    bool ok=true;
    do {
        const std::string child=info.cFileName;
        if(child=="."||child=="..")continue;
        if(children.size()>=256) {ok=false;break;}
        children.push_back(child);
    } while(FindNextFileA(handle,&info));
    if(ok && GetLastError()!=ERROR_NO_MORE_FILES)ok=false;
    FindClose(handle);
    if(!ok)return false;
#else
    for(fs::directory_iterator it(from,ec),end;!ec&&it!=end;it.increment(ec)) {
        if(children.size()>=256)return false;
        children.push_back(it->path().filename().string());
    }
    if(ec)return false;
#endif
    for(size_t i=0;i<children.size();++i)
        if(!PlanItem(from,children[i],to,depth+1,steps))return false;
    return true;
}
bool FileWorkspace::Copy(const std::string& sourceDir,const std::string& name,
                         const std::string& destDir) const {
    // Legacy bounded synchronous API for callers that do not drive Tick.
    std::string from,to;
    if(!PrepareCopy(sourceDir,name,destDir,from,to))return false;
    std::ifstream input(from.c_str(),std::ios::binary|std::ios::ate);
    if(!input)return false;
    const std::streamoff size=input.tellg();
    input.close();
    if(size<0 || size>8*1024*1024)return false;
    FileTransfer job;
    if(!job.Start(*this,sourceDir,name,destDir))return false;
    while(job.Status()==FileTransfer::Running)job.Tick();
    return job.Status()==FileTransfer::Completed;
}
FileTransfer::FileTransfer():state_(Idle),done_(0),total_(0)
#if defined(_XBOX) || defined(_WIN32)
    ,output_(INVALID_HANDLE_VALUE)
#else
    ,output_(-1)
#endif
{}
FileTransfer::~FileTransfer() {Cleanup();}
void FileTransfer::Cleanup() {
    if(input_.is_open())input_.close();
#if defined(_XBOX) || defined(_WIN32)
    if(output_!=INVALID_HANDLE_VALUE) {CloseHandle(output_);output_=INVALID_HANDLE_VALUE;}
#else
    if(output_!=-1) {close(output_);output_=-1;}
#endif
    if(!temporary_.empty()) {
#if defined(_XBOX) || defined(_WIN32)
        DeleteFileA(temporary_.c_str());
#else
        unlink(temporary_.c_str());
#endif
        temporary_.clear();
    }
}
void FileTransfer::Fail(const char* reason) {Cleanup();error_=reason;state_=Failed;}
bool FileTransfer::Start(const FileWorkspace& workspace,const std::string& sourceDir,
                         const std::string& name,const std::string& destDir) {
    if(state_==Running)return false;
    Cleanup();
    state_=Idle;done_=total_=0;name_=name;error_.clear();
    std::string from;
    if(!workspace.PrepareCopy(sourceDir,name,destDir,from,target_)) {
        Fail("Origem ou destino invalido; nao sobrescreve arquivos.");return false;
    }
    input_.clear();input_.open(from.c_str(),std::ios::binary|std::ios::ate);
    if(!input_) {Fail("Nao foi possivel abrir a origem.");return false;}
    const std::streamoff size=input_.tellg();
    if(size<0) {Fail("Tamanho nao disponivel.");return false;}
    total_=static_cast<unsigned long long>(size);
    input_.seekg(0,std::ios::beg);
    if(!input_) {Fail("Falha ao posicionar a origem.");return false;}
    for(int i=0;i<32;++i) {
        char temp[32];
        std::sprintf(temp,"AXCOPY-%04d.part",i);
        const std::string candidate=FileWorkspace::Join(destDir,temp);
#if defined(_XBOX) || defined(_WIN32)
        output_=CreateFileA(candidate.c_str(),GENERIC_WRITE,0,0,CREATE_NEW,
                           FILE_ATTRIBUTE_NORMAL,0);
        if(output_==INVALID_HANDLE_VALUE)continue;
#else
        output_=open(candidate.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
        if(output_==-1)continue;
#endif
        temporary_=candidate;state_=Running;return true;
    }
    Fail("Nao foi possivel reservar arquivo temporario.");return false;
}
unsigned FileTransfer::Percent() const {
    if(state_==Completed)return 100;
    if(!total_)return 0;
    const double value=100.0*static_cast<double>(done_)/static_cast<double>(total_);
    return value>=99.0?99:static_cast<unsigned>(value);
}
void FileTransfer::Cancel() {
    if(state_==Running) {Cleanup();state_=Cancelled;}
}
void FileTransfer::Tick() {
    if(state_!=Running)return;
    if(done_<total_) {
        char buffer[64*1024];
        const unsigned long long remaining=total_-done_;
        const size_t want=remaining<sizeof(buffer)?static_cast<size_t>(remaining):sizeof(buffer);
        input_.read(buffer,static_cast<std::streamsize>(want));
        if(input_.gcount()!=static_cast<std::streamsize>(want)) {
            Fail("Origem removida ou alterada durante a copia.");return;
        }
#if defined(_XBOX) || defined(_WIN32)
        DWORD wrote=0;
        if(!WriteFile(output_,buffer,static_cast<DWORD>(want),&wrote,0) || wrote!=want) {
            Fail("Falha de escrita ou espaco insuficiente.");return;
        }
#else
        size_t written=0;
        while(written<want) {
            const ssize_t n=write(output_,buffer+written,want-written);
            if(n<0 && errno==EINTR)continue;
            if(n<=0) {Fail("Falha de escrita ou espaco insuficiente.");return;}
            written+=static_cast<size_t>(n);
        }
#endif
        done_+=want;
        return;
    }
    if(input_.peek()!=std::char_traits<char>::eof() || input_.bad()) {
        Fail("Origem mudou durante a copia.");return;
    }
    input_.close();
#if defined(_XBOX) || defined(_WIN32)
    const bool flushed=FlushFileBuffers(output_)!=0;
    const bool closed=CloseHandle(output_)!=0;
    output_=INVALID_HANDLE_VALUE;
    if(!flushed || !closed) {Fail("Falha ao finalizar a escrita.");return;}
    if(!MoveFileA(temporary_.c_str(),target_.c_str())) {
        Fail("Destino ocupado ou falha ao concluir.");return;
    }
#else
    const bool flushed=fsync(output_)==0;
    const bool closed=close(output_)==0;
    output_=-1;
    if(!flushed || !closed) {Fail("Falha ao finalizar a escrita.");return;}
#ifdef __XSDK__
    if(!renameNoReplace(temporary_,target_)){
        Fail("Destino ocupado ou falha ao concluir sem sobrescrever.");return;
    }
#else
    // Atomic no-replace publication on filesystems with hard-link support.
    if(link(temporary_.c_str(),target_.c_str())!=0) {
        Fail("Destino ocupado ou sistema sem suporte a publicacao segura.");return;
    }
    unlink(temporary_.c_str());
#endif
#endif
    temporary_.clear();state_=Completed;
}
