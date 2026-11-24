#include "FileWorkspace.h"
#include <cctype>
#include <cstdio>
#include <fstream>
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
#endif
}
bool FileWorkspace::ValidName(const std::string& name) {
    if(name.empty() || name.size()>42 || name=="." || name==".." ||
       name[0]==' ' || name[name.size()-1]==' ' || name[name.size()-1]=='.')
        return false;
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
std::string FileWorkspace::WorkspaceFor(const std::string& root) const {
    for(size_t i=0;i<roots_.size();++i)
        if(key(root)==key(roots_[i]))return workspaces_[i];
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
bool FileWorkspace::Copy(const std::string& sourceDir,const std::string& name,
                         const std::string& destDir) const {
    if(!CanBrowse(sourceDir) || !CanWrite(destDir) || !ValidName(name))return false;
    const std::string from=Join(sourceDir,name),to=Join(destDir,name);
    if(!Allowed(from,roots_) || !Allowed(to,workspaces_) || !exists(from) ||
       directory(from) || exists(to))return false;
    // Alpha: bounded synchronous copies only, never overwrite.
    std::ifstream input(from.c_str(),std::ios::binary|std::ios::ate);
    if(!input)return false;
    const std::streamoff size=input.tellg();
    input.close();
    if(size<0 || size>8*1024*1024)return false;
#ifdef _XBOX
    return CopyFileA(from.c_str(),to.c_str(),TRUE)!=0;
#else
    std::error_code ec;
    return fs::copy_file(from,to,fs::copy_options::none,ec) && !ec;
#endif
}
