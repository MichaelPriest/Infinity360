#include "FileWorkspace.h"
#include <cctype>
#include <cstdio>
#include <fstream>
#include <cerrno>
#include <set>
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
#ifdef _XBOX
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
    const std::string p=key(path),r=key(root);
    if(p==r)return true;
    if(p.size()<=r.size() || p.compare(0,r.size(),r)!=0)return false;
    return r=="/" || p[r.size()]=='/' || p[r.size()]=='\\';
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
#ifndef _XBOX
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
}

FileWorkspace::FileWorkspace(const std::string& appDirectory) {
    roots_.push_back(appDirectory);
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
bool FileWorkspace::CanReadPersonalFile(const std::string& path) const {
    if(!Allowed(path,workspaces_) || !exists(path) || directory(path))return false;
#ifndef _XBOX
    std::error_code ec;
    return fs::is_regular_file(path,ec) && !ec;
#else
    return true;
#endif
}
std::string FileWorkspace::WorkspaceFor(const std::string& root) const {
    for(size_t i=0;i<roots_.size();++i)
        if(key(root)==key(roots_[i]))return i<workspaces_.size()?workspaces_[i]:"";
    return "";
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
    // A same-filesystem hard link reserves a new name without clobbering it.
    // If source unlink fails, leave both names rather than risk deleting data.
    std::error_code ec;
    if(!fs::is_regular_file(from,ec)||ec)return false;
    if(link(from.c_str(),to.c_str())!=0)return false;
    return unlink(from.c_str())==0;
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
    // Atomic no-replace publication on filesystems with hard-link support.
    if(link(temporary_.c_str(),target_.c_str())!=0) {
        Fail("Destino ocupado ou sistema sem suporte a publicacao segura.");return;
    }
    unlink(temporary_.c_str());
#endif
    temporary_.clear();state_=Completed;
}
