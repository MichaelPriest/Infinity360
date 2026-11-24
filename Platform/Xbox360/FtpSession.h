#pragma once
#include "FileWorkspace.h"
#include <string>

// Transport-independent FTP command session. No sockets or data channel.
// One instance per client; credentials are supplied explicitly by the owner.
class FtpSession {
public:
    FtpSession(const FileWorkspace& workspace,const std::string& personalRoot,
               const std::string& username,const std::string& password);
    std::string Greeting() const;
    std::string Command(const std::string& line);
    // Validated read-only paths for a transport adapter. Never expose these
    // physical paths to a remote client.
    bool DataDirectory(const std::string& arg,std::string& path) const;
    bool DataFile(const std::string& arg,std::string& path) const;
    bool Authenticated() const { return authenticated_; }
    bool Closed() const { return closed_; }
    static bool ValidCredentials(const std::string& user,const std::string& password);
private:
    FtpSession(const FtpSession&);
    FtpSession& operator=(const FtpSession&);
    bool ResolveDirectory(const std::string& argument,std::string& relative) const;
    std::string Physical(const std::string& relative) const;
    const FileWorkspace& workspace_;
    std::string root_,user_,password_,cwd_;
    bool enabled_,userAccepted_,authenticated_,closed_;
    unsigned failures_;
};
