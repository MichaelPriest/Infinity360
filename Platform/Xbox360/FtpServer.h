#pragma once
#include "FileWorkspace.h"
#include <string>

// Nonblocking, single-client passive FTP adapter; write access opt-in.
// No automatic startup, no active mode, no external network services.
class FtpServer {
public:
    explicit FtpServer(const FileWorkspace& workspace);
    ~FtpServer();
    bool Start(const std::string& root,const std::string& address,unsigned port,
               const std::string& user,const std::string& password,bool allowUpload=false,unsigned long long uploadLimit=32ULL*1024*1024);
    void Stop();
    bool Tick();
    bool Running() const;
    bool Connected() const;
    bool UploadEnabled() const;
    unsigned Port() const;
    const std::string& Error() const {return error_;}
private:
    FtpServer(const FtpServer&);
    FtpServer& operator=(const FtpServer&);
    struct Impl;
    Impl* impl_;
    const FileWorkspace& workspace_;
    std::string error_;
};
