#pragma once
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMB_MENU)
#include "FileWorkspace.h"
#include "SmbBrowser.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cerrno>
#include <cstdio>
#include <fstream>

// Exclusive staging in an existing personal workspace, never a system mount.
class SmbConsoleStage {
public:
    ~SmbConsoleStage(){Cancel();}
    SmbConsoleStage()=default;
    SmbConsoleStage(const SmbConsoleStage&)=delete;
    SmbConsoleStage& operator=(const SmbConsoleStage&)=delete;
    bool Start(const FileWorkspace& ws,const std::string& root,const SmbBrowser::CopyPlan& plan){
        Cancel();
        const std::string parent=ws.WorkspaceFor(root);
        if(parent.empty()||!ws.CanWrite(parent)||!SmbBrowser::LocalName(plan.name)||
           !FileWorkspace::ValidName(plan.name)||plan.bytes>0xffffffffULL)return false;
        target_=FileWorkspace::Join(parent,plan.name);
        struct stat st{};
        if(stat(target_.c_str(),&st)==0||errno!=ENOENT){Cancel();return false;}
        total_=plan.bytes;
        for(unsigned i=0;i<32;++i){
            char name[32];std::snprintf(name,sizeof name,"AXCOPY-SMB-%04u.part",i);
            const auto candidate=FileWorkspace::Join(parent,name);
            fd_=open(candidate.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
            if(fd_>=0){temp_=candidate;return true;}
            if(errno!=EEXIST)break;
        }
        Cancel();return false;
    }
    bool Write(const uint8_t* data,size_t n){
        if(fd_<0||!data||done_>total_||n>total_-done_)return false;
        size_t at=0;
        while(at<n){
            const auto w=write(fd_,data+at,n-at);
            if(w<0&&errno==EINTR)continue;
            if(w<=0)return false;
            at+=static_cast<size_t>(w);
        }
        done_+=n;return true;
    }
    bool Publish(const FileWorkspace& ws){
        if(fd_<0||done_!=total_)return false;
        const bool flushed=fsync(fd_)==0;
        const bool closed=close(fd_)==0;fd_=-1;
        if(!flushed||!closed||!ws.PublishStaged(temp_,target_))return false;
        temp_.clear();return true;
    }
    void Cancel(){
        if(fd_>=0){close(fd_);fd_=-1;}
        if(!temp_.empty())unlink(temp_.c_str());
        temp_.clear();target_.clear();total_=done_=0;
    }
private:
    int fd_=-1;uint64_t done_=0,total_=0;std::string temp_,target_;
};
#endif
