#include "FtpSession.h"
#include <cctype>
#include <vector>

namespace {
bool printable(const std::string& s) {
    for(size_t i=0;i<s.size();++i) {
        const unsigned char ch=(unsigned char)s[i];
        if(ch<32 || ch>126)return false;
    }
    return true;
}
bool equalSecret(const std::string& a,const std::string& b) {
    // Compare all bytes of the bounded credentials without early exit.
    unsigned diff=static_cast<unsigned>(a.size()^b.size());
    for(size_t i=0;i<64;++i) {
        const unsigned x=i<a.size()?(unsigned char)a[i]:0;
        const unsigned y=i<b.size()?(unsigned char)b[i]:0;
        diff|=x^y;
    }
    return diff==0;
}
}

bool FtpSession::ValidCredentials(const std::string& user,const std::string& pass) {
    if(user.empty() || user.size()>32 || pass.size()<12 || pass.size()>64 ||
       !printable(user) || !printable(pass))return false;
    for(size_t i=0;i<user.size();++i) {
        const unsigned char ch=(unsigned char)user[i];
        if(!(std::isalnum(ch) || ch=='_' || ch=='-'))return false;
    }
    std::string lower=user;
    for(size_t i=0;i<lower.size();++i)lower[i]=(char)std::tolower((unsigned char)lower[i]);
    return lower!="anonymous" && !equalSecret(user,pass);
}

FtpSession::FtpSession(const FileWorkspace& workspace,const std::string& personalRoot,
                       const std::string& username,const std::string& password)
    :workspace_(workspace),root_(personalRoot),user_(username),password_(password),
     cwd_("/"),enabled_(workspace.CanWrite(personalRoot) &&
     ValidCredentials(username,password)),userAccepted_(false),
     authenticated_(false),closed_(!enabled_),failures_(0) {}

std::string FtpSession::Greeting() const {
    return enabled_?"220 AuroraX FTP command engine ready":"421 Service unavailable";
}
std::string FtpSession::Physical(const std::string& relative) const {
    std::string result=root_;
    size_t at=1;
    while(at<relative.size()) {
        size_t end=relative.find('/',at);
        if(end==std::string::npos)end=relative.size();
        result=FileWorkspace::Join(result,relative.substr(at,end-at));
        at=end+1;
    }
    return result;
}
bool FtpSession::ResolveDirectory(const std::string& argument,std::string& relative) const {
    if(argument.empty() || argument.size()>200 || !printable(argument))return false;
    // FTP paths use slash; reject drive prefixes and alternate separators.
    if(argument.find('\\')!=std::string::npos || argument.find(':')!=std::string::npos)
        return false;
    relative=argument[0]=='/'?"/":cwd_;
    size_t at=argument[0]=='/'?1:0;
    while(at<argument.size()) {
        size_t end=argument.find('/',at);
        if(end==std::string::npos)end=argument.size();
        const std::string part=argument.substr(at,end-at);
        if(part=="..") {
            if(relative=="/")return false;
            const size_t last=relative.find_last_of('/');
            relative=last==0?"/":relative.substr(0,last);
        } else if(!part.empty() && part!=".") {
            if(!FileWorkspace::ValidName(part))return false;
            if(relative!="/")relative+="/";
            relative+=part;
        }
        at=end+1;
    }
    return relative.size()<=200 && workspace_.CanWrite(Physical(relative));
}
bool FtpSession::DataDirectory(const std::string& arg,std::string& path) const {
    path.clear();
    if(!authenticated_ || closed_ || !workspace_.CanWrite(root_))return false;
    std::string relative;
    if(!ResolveDirectory(arg.empty()?".":arg,relative))return false;
    path=Physical(relative);
    return true;
}
bool FtpSession::DataFile(const std::string& arg,std::string& path) const {
    path.clear();
    if(arg.empty() || arg.size()>200 || !printable(arg) ||
       arg.find('\\')!=std::string::npos || arg.find(':')!=std::string::npos)return false;
    const size_t slash=arg.find_last_of('/');
    const std::string name=slash==std::string::npos?arg:arg.substr(slash+1);
    if(!FileWorkspace::ValidName(name))return false;
    std::string parent;
    if(!DataDirectory(slash==std::string::npos?".":slash==0?"/":arg.substr(0,slash),parent))
        return false;
    const std::string candidate=FileWorkspace::Join(parent,name);
    if(!workspace_.CanReadPersonalFile(candidate))return false;
    path=candidate;
    return true;
}
std::string FtpSession::Command(const std::string& line) {
    if(closed_)return "421 Service unavailable";
    if(line.empty() || line.size()>512 || !printable(line))return "500 Invalid command";
    const size_t separator=line.find(' ');
    std::string cmd=line.substr(0,separator);
    const std::string arg=separator==std::string::npos?"":line.substr(separator+1);
    for(size_t i=0;i<cmd.size();++i)cmd[i]=(char)std::toupper((unsigned char)cmd[i]);
    if(cmd=="QUIT") {authenticated_=false;closed_=true;return "221 Goodbye";}
    if(cmd=="USER") {
        authenticated_=false;cwd_="/";
        userAccepted_=arg.size()<=32 && equalSecret(arg,user_);
        return "331 Password required";
    }
    if(cmd=="PASS") {
        const bool match=userAccepted_ && arg.size()<=64 && equalSecret(arg,password_);
        userAccepted_=false;
        if(match) {authenticated_=true;failures_=0;return "230 Logged in";}
        authenticated_=false;
        if(++failures_>=3) {closed_=true;return "421 Too many failed attempts";}
        return "530 Login incorrect";
    }
    if(!authenticated_)return "530 Login required";
    if(!workspace_.CanWrite(root_) || !workspace_.CanWrite(Physical(cwd_))) {
        authenticated_=false;closed_=true;return "421 Workspace unavailable";
    }
    if(cmd=="SYST")return "215 UNIX Type: L8";
    if(cmd=="FEAT")return "211 No extensions enabled";
    if(cmd=="NOOP")return "200 OK";
    if(cmd=="PWD" || cmd=="XPWD")return "257 \""+cwd_+"\"";
    if(cmd=="TYPE")return arg=="I"?"200 Binary mode":"504 Only binary mode supported";
    if(cmd=="CWD" || cmd=="CDUP") {
        std::string next;
        if(cmd=="CDUP" && cwd_=="/")return "250 Directory unchanged";
        if(!ResolveDirectory(cmd=="CDUP"?"..":arg,next))return "550 Directory unavailable";
        cwd_=next;return "250 Directory changed";
    }
    // Never allow active-mode connections, mutation, or unaudited data channels.
    if(cmd=="PORT" || cmd=="EPRT" || cmd=="STOR" || cmd=="APPE" ||
       cmd=="DELE" || cmd=="RMD" || cmd=="MKD" || cmd=="RNFR" ||
       cmd=="RNTO" || cmd=="SITE")return "502 Operation disabled";
    if(cmd=="PASV" || cmd=="EPSV" || cmd=="LIST" || cmd=="NLST" ||
       cmd=="RETR" || cmd=="SIZE")return "502 Data transport not integrated";
    return "502 Command not implemented";
}
