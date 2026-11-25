#include "FtpServer.h"
#include "FtpSession.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cctype>
#if defined(_XBOX) || defined(__linux__)
#ifdef _XBOX
#include <xtl.h>
#include <winsockx.h>
#include <xnet.h>
typedef SOCKET Sock;
typedef int SockLen;
const Sock Invalid=INVALID_SOCKET;
#else
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>
typedef int Sock;
typedef socklen_t SockLen;
const Sock Invalid=-1;
#endif
namespace {
void closeSock(Sock& s) {
    if(s==Invalid)return;
#ifdef _XBOX
    closesocket(s);
#else
    close(s);
#endif
    s=Invalid;
}
bool nonblocking(Sock s) {
#ifdef _XBOX
    u_long mode=1;return ioctlsocket(s,FIONBIO,&mode)==0;
#else
    int flags=fcntl(s,F_GETFL,0);return flags>=0&&fcntl(s,F_SETFL,flags|O_NONBLOCK)==0;
#endif
}
bool pending() {
#ifdef _XBOX
    return WSAGetLastError()==WSAEWOULDBLOCK;
#else
    return errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR;
#endif
}
unsigned ticks() {
#ifdef _XBOX
    return GetTickCount();
#else
    struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);
    return static_cast<unsigned>(t.tv_sec*1000ULL+t.tv_nsec/1000000);
#endif
}
bool localAddress(unsigned long ip) {
    const unsigned long h=ntohl(ip);
    return (h>>24)==10||(h>>20)==0xac1||(h>>16)==0xc0a8
#ifndef _XBOX
        ||(h>>24)==127
#endif
        ;
}
Sock listener(unsigned long address,unsigned port,unsigned& assigned) {
    Sock s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
    if(s==Invalid)return s;
    sockaddr_in a;std::memset(&a,0,sizeof a);
    a.sin_family=AF_INET;a.sin_addr.s_addr=address;a.sin_port=htons(static_cast<unsigned short>(port));
    if(!nonblocking(s)||bind(s,reinterpret_cast<sockaddr*>(&a),sizeof a)!=0||listen(s,1)!=0) {
        closeSock(s);return Invalid;
    }
    SockLen len=sizeof a;
    if(getsockname(s,reinterpret_cast<sockaddr*>(&a),&len)!=0) {closeSock(s);return Invalid;}
    assigned=ntohs(a.sin_port);return s;
}
int sendSome(Sock socket,const char* data,unsigned size) {
#ifdef _XBOX
    return send(socket,data,static_cast<int>(size),0);
#else
    return static_cast<int>(send(socket,data,size,MSG_NOSIGNAL));
#endif
}
std::string num(unsigned long long n) {std::ostringstream out;out<<n;return out.str();}
bool listEntry(FtpSession& ftp,const std::string& name,bool names,std::string& result) {
    if(!FileWorkspace::ValidName(name))return true;
    std::string path;
    bool dir=ftp.DataDirectory(name,path);
    if(!dir&&!ftp.DataFile(name,path))return true;
    unsigned long long size=0;
    if(!dir) {
        std::ifstream in(path.c_str(),std::ios::binary|std::ios::ate);
        if(!in||in.tellg()<0)return false;
        size=static_cast<unsigned long long>(in.tellg());
    }
    if(!names)result+=(dir?"dr-xr-xr-x":"-r--r--r--")+std::string(" 1 owner owner ")+num(size)+" Jan 01 2000 ";
    result+=name+"\r\n";
    return result.size()<=65536;
}
bool directoryListing(FtpSession& ftp,const std::string& path,bool names,std::string& result) {
    unsigned count=0;
#ifdef _XBOX
    WIN32_FIND_DATAA info;
    HANDLE h=FindFirstFileA(FileWorkspace::Join(path,"*").c_str(),&info);
    if(h==INVALID_HANDLE_VALUE)return GetLastError()==ERROR_FILE_NOT_FOUND;
    bool ok=true;
    do {
        if(++count>512||!listEntry(ftp,info.cFileName,names,result)){ok=false;break;}
    } while(FindNextFileA(h,&info));
    if(ok&&GetLastError()!=ERROR_NO_MORE_FILES)ok=false;
    FindClose(h);return ok;
#else
    DIR* d=opendir(path.c_str());if(!d)return false;
    bool ok=true;
    for(;;) {
        errno=0;struct dirent* e=readdir(d);
        if(!e){if(errno)ok=false;break;}
        if(++count>512||!listEntry(ftp,e->d_name,names,result)){ok=false;break;}
    }
    closedir(d);return ok;
#endif
}
}
struct FtpServer::Impl {
    Sock server,control,passive,data;
    unsigned long address,peer;
    unsigned port,last,disconnected;
    bool network,sockets,binary,closing,transferring,listing;
    FtpSession* session;
    std::string root,user,password,input,reply,payload;
    std::ifstream file;
    size_t offset;
    Impl():server(Invalid),control(Invalid),passive(Invalid),data(Invalid),address(0),peer(0),
        port(0),last(0),disconnected(0),network(false),sockets(false),binary(false),closing(false),
        transferring(false),listing(false),session(0),offset(0){}
    void clearData() {
        closeSock(passive);closeSock(data);file.close();file.clear();
        payload.clear();offset=0;transferring=false;listing=false;
    }
    void clearClient() {
        clearData();closeSock(control);delete session;session=0;input.clear();reply.clear();
        closing=false;binary=false;disconnected=ticks();
    }
    ~Impl() {
        clearClient();closeSock(server);
#ifdef _XBOX
        if(sockets)WSACleanup();
        if(network)XNetCleanup();
#endif
    }
    void respond(const std::string& value){reply=value+"\r\n";}
    void finish(bool success){clearData();respond(success?"226 Transfer complete":"426 Transfer interrupted");}
    void command(const std::string& line) {
        const size_t split=line.find(' ');
        std::string verb=line.substr(0,split);
        const std::string arg=split==std::string::npos?"":line.substr(split+1);
        for(size_t i=0;i<verb.size();++i)verb[i]=(char)std::toupper((unsigned char)verb[i]);
        bool valid=!line.empty()&&line.size()<=512;
        for(size_t i=0;i<line.size();++i)if((unsigned char)line[i]<32||(unsigned char)line[i]>126)valid=false;
        if(!valid){respond("500 Invalid command");return;}
        if(!session->Authenticated()||verb=="USER"||verb=="PASS"||verb=="QUIT") {
            clearData();binary=false;respond(session->Command(line));closing=session->Closed();return;
        }
        if(verb=="TYPE") {
            if(arg=="A"||arg=="I"){binary=arg=="I";respond("200 Type set");}
            else respond("504 Unsupported type");
            return;
        }
        if(verb=="PASV"||verb=="EPSV") {
            clearData();unsigned p=0;passive=listener(address,0,p);
            if(passive==Invalid){respond("425 Cannot open passive listener");return;}
            const unsigned long a=ntohl(address);
            respond(verb=="EPSV"?"229 Entering Extended Passive Mode (|||"+num(p)+"|)":
                "227 Entering Passive Mode ("+num((a>>24)&255)+","+num((a>>16)&255)+","+
                num((a>>8)&255)+","+num(a&255)+","+num(p/256)+","+num(p%256)+")");
            return;
        }
        if(verb=="SIZE") {
            std::string path;
            if(!binary||!session->DataFile(arg,path)){respond("550 File unavailable");return;}
            std::ifstream in(path.c_str(),std::ios::binary|std::ios::ate);
            if(!in||in.tellg()<0){respond("550 File unavailable");return;}
            respond("213 "+num(static_cast<unsigned long long>(in.tellg())));return;
        }
        if(verb=="LIST"||verb=="NLST"||verb=="RETR") {
            if(passive==Invalid){respond("425 Use passive mode first");return;}
            std::string path;
            listing=verb!="RETR";
            bool ok=listing?(arg.empty()&&session->DataDirectory("",path)&&
                              directoryListing(*session,path,verb=="NLST",payload)):
                             (binary&&session->DataFile(arg,path));
            if(ok&&!listing) {file.open(path.c_str(),std::ios::binary);ok=!!file;}
            if(!ok){clearData();respond("550 Path unavailable or listing limit");return;}
            transferring=true;offset=0;respond("150 Opening data connection");return;
        }
        respond(session->Command(line));closing=session->Closed();
    }
    void transferStep() {
        if(data==Invalid) {
            sockaddr_in a;SockLen len=sizeof a;
            Sock s=accept(passive,reinterpret_cast<sockaddr*>(&a),&len);
            if(s==Invalid) {if(!pending())finish(false);return;}
            if(a.sin_addr.s_addr!=peer||!nonblocking(s)){closeSock(s);return;}
            data=s;closeSock(passive);last=ticks();
        }
        if(offset==payload.size()) {
            payload.clear();offset=0;
            if(listing){finish(true);return;}
            char buffer[32768];file.read(buffer,sizeof buffer);
            const std::streamsize n=file.gcount();
            if(n<=0){finish(file.eof()&&!file.bad());return;}
            payload.assign(buffer,static_cast<size_t>(n));
        }
        const int n=sendSome(data,payload.data()+offset,static_cast<unsigned>(payload.size()-offset));
        if(n>0){offset+=static_cast<size_t>(n);last=ticks();}
        else if(n==0||!pending())finish(false);
    }
};
FtpServer::FtpServer(const FileWorkspace& workspace):impl_(0),workspace_(workspace){}
FtpServer::~FtpServer(){Stop();}
void FtpServer::Stop(){delete impl_;impl_=0;}
bool FtpServer::Running()const{return impl_!=0;}
bool FtpServer::Connected()const{return impl_&&impl_->control!=Invalid;}
unsigned FtpServer::Port()const{return impl_?impl_->port:0;}
bool FtpServer::Start(const std::string& root,const std::string& addr,unsigned port,
                      const std::string& user,const std::string& password) {
    Stop();error_.clear();
    if(!workspace_.CanWrite(root)||!FtpSession::ValidCredentials(user,password)||port>65535) {
        error_="Workspace ou credenciais invalidos.";return false;
    }
    const unsigned long ip=inet_addr(addr.c_str());
    if(!localAddress(ip)){error_="IPv4 LAN privado necessario.";return false;}
    Impl* p=new Impl;
#ifdef _XBOX
    XNetStartupParams params;std::memset(&params,0,sizeof params);
    params.cfgSizeOfStruct=sizeof params;
    if(XNetStartup(&params)!=0){delete p;error_="XNet indisponivel.";return false;}
    p->network=true;
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){delete p;error_="Winsock indisponivel.";return false;}
    p->sockets=true;
#endif
    p->address=ip;p->root=root;p->user=user;p->password=password;
    p->server=listener(ip,port,p->port);
    if(p->server==Invalid){delete p;error_="Nao foi possivel abrir porta FTP.";return false;}
    p->disconnected=ticks()-2000;impl_=p;return true;
}
bool FtpServer::Tick() {
    if(!impl_)return false;
    Impl& p=*impl_;
    const unsigned now=ticks();
    if(p.control==Invalid) {
        if(now-p.disconnected<2000)return false;
        sockaddr_in a;SockLen len=sizeof a;
        Sock client=accept(p.server,reinterpret_cast<sockaddr*>(&a),&len);
        if(client==Invalid)return false;
        if(!localAddress(a.sin_addr.s_addr)||!nonblocking(client)){closeSock(client);return false;}
        p.control=client;p.peer=a.sin_addr.s_addr;p.last=now;
        p.session=new FtpSession(workspace_,p.root,p.user,p.password);
        p.respond(p.session->Greeting());p.closing=p.session->Closed();return true;
    }
    if(now-p.last>45000){p.clearClient();return true;}
    if(!p.reply.empty()) {
        const int n=sendSome(p.control,p.reply.data(),static_cast<unsigned>(p.reply.size()));
        if(n>0){p.reply.erase(0,n);p.last=now;}
        else if(n==0||!pending()){p.clearClient();return true;}
        return false;
    }
    if(p.closing){p.clearClient();return true;}
    if(p.transferring){p.transferStep();return false;}
    size_t end=p.input.find("\r\n");
    if(end==std::string::npos) {
        char buffer[512];const int n=recv(p.control,buffer,sizeof buffer,0);
        if(n==0||(n<0&&!pending())){p.clearClient();return true;}
        if(n<0)return false;
        p.input.append(buffer,static_cast<size_t>(n));p.last=now;
        end=p.input.find("\r\n");
        if(end==std::string::npos&&p.input.size()>512){p.clearClient();return true;}
    }
    if(end!=std::string::npos) {
        const std::string line=p.input.substr(0,end);p.input.erase(0,end+2);p.command(line);
    }
    return false;
}
#else
FtpServer::FtpServer(const FileWorkspace& workspace):impl_(0),workspace_(workspace){}
FtpServer::~FtpServer(){}
bool FtpServer::Start(const std::string&,const std::string&,unsigned,const std::string&,const std::string&){
    error_="Provider FTP indisponivel nesta plataforma.";return false;
}
void FtpServer::Stop(){}
bool FtpServer::Tick(){return false;}
bool FtpServer::Running()const{return false;}
bool FtpServer::Connected()const{return false;}
unsigned FtpServer::Port()const{return 0;}
#endif
