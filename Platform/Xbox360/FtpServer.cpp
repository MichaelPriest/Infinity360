#include "FtpServer.h"
#include "FtpSession.h"
#include "StorageInfo.h"
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <cctype>
#if defined(_XBOX) || defined(__linux__) || defined(__XSDK__)
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
#include <sys/stat.h>
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
#if !defined(_XBOX) && !defined(__XSDK__)
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
#elif defined(__XSDK__)
    return static_cast<int>(send(socket,data,size,0));
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
// Exact-size, exclusive upload. Temp names cannot be accessed through FTP.
#ifdef __XSDK__
class Upload {
public:
    ~Upload(){Cancel();}
    bool Open(FtpSession& ftp,const std::string& argument,unsigned long long expected){
        Cancel();
        const size_t slash=argument.find_last_of('/');
        const std::string name=slash==std::string::npos?argument:argument.substr(slash+1);
        relative_=slash==std::string::npos?".":slash==0?"/":argument.substr(0,slash);
        if(!FileWorkspace::ValidName(name)||!ftp.DataDirectory(relative_,parent_))return false;
        target_=FileWorkspace::Join(parent_,name);
        struct stat st{};
        if(stat(target_.c_str(),&st)==0||errno!=ENOENT)return false;
        expected_=expected;
        for(unsigned i=0;i<32;++i){
            char candidate[32];std::sprintf(candidate,"AXCOPY-FTP-%04u.part",i);
            const std::string path=FileWorkspace::Join(parent_,candidate);
            handle_=open(path.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
            if(handle_>=0){temp_=path;return true;}
        }
        return false;
    }
    bool Write(const char* bytes,unsigned n){
        if(handle_<0||n>expected_-received_)return false;
        size_t at=0;
        while(at<n){
            const ssize_t r=write(handle_,bytes+at,n-at);
            if(r<0&&errno==EINTR)continue;
            if(r<=0)return false;
            at+=static_cast<size_t>(r);
        }
        received_+=n;return true;
    }
    bool Publish(FtpSession& ftp,const FileWorkspace& workspace){
        std::string current;
        if(handle_<0||received_!=expected_||!ftp.DataDirectory(relative_,current)||current!=parent_)return false;
        const bool flushed=fsync(handle_)==0;
        const bool closed=close(handle_)==0;handle_=-1;
        if(!flushed||!closed||!workspace.PublishStaged(temp_,target_))return false;
        temp_.clear();return true;
    }
    void Cancel(){
        if(handle_>=0){close(handle_);handle_=-1;}
        if(!temp_.empty())unlink(temp_.c_str());
        temp_.clear();target_.clear();parent_.clear();relative_.clear();expected_=received_=0;
    }
private:
    int handle_=-1;
    unsigned long long expected_=0,received_=0;
    std::string parent_,target_,temp_,relative_;
};
#else
class Upload {
public:
    Upload():expected_(0),received_(0)
#ifdef _XBOX
        ,handle_(INVALID_HANDLE_VALUE)
#else
        ,dir_(-1),handle_(-1)
#endif
    {}
    ~Upload(){Cancel();}
    bool Open(FtpSession& ftp,const std::string& argument,unsigned long long expected) {
        Cancel();
        const size_t slash=argument.find_last_of('/');
        const std::string name=slash==std::string::npos?argument:argument.substr(slash+1);
        relative_=slash==std::string::npos?".":slash==0?"/":argument.substr(0,slash);
        if(!FileWorkspace::ValidName(name)||!ftp.DataDirectory(relative_,parent_))return false;
        target_=name;expected_=expected;received_=0;
#ifdef _XBOX
        const std::string destination=FileWorkspace::Join(parent_,target_);
        if(GetFileAttributesA(destination.c_str())!=INVALID_FILE_ATTRIBUTES)return false;
#else
        dir_=open(parent_.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
        if(dir_<0)return false;
        struct stat st;
        if(fstatat(dir_,target_.c_str(),&st,AT_SYMLINK_NOFOLLOW)==0||errno!=ENOENT){Cancel();return false;}
#endif
        for(unsigned i=0;i<32;++i) {
            char candidate[32];std::sprintf(candidate,"AXCOPY-FTP-%04u.part",i);
#ifdef _XBOX
            handle_=CreateFileA(FileWorkspace::Join(parent_,candidate).c_str(),GENERIC_WRITE,0,0,
                                CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
            if(handle_==INVALID_HANDLE_VALUE)continue;
#else
            handle_=openat(dir_,candidate,O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
            if(handle_<0)continue;
#endif
            temp_=candidate;return true;
        }
        Cancel();return false;
    }
    bool Write(const char* bytes,unsigned n) {
        if(n>expected_-received_)return false;
#ifdef _XBOX
        DWORD wrote=0;
        if(!WriteFile(handle_,bytes,n,&wrote,0)||wrote!=n)return false;
#else
        size_t at=0;
        while(at<n) {
            const ssize_t result=write(handle_,bytes+at,n-at);
            if(result<0&&errno==EINTR)continue;
            if(result<=0)return false;
            at+=static_cast<size_t>(result);
        }
#endif
        received_+=n;return true;
    }
    bool Publish(FtpSession& ftp) {
        std::string current;
        if(received_!=expected_||!ftp.DataDirectory(relative_,current)||current!=parent_)return false;
#ifdef _XBOX
        const bool flushed=FlushFileBuffers(handle_)!=0;
        const bool closed=CloseHandle(handle_)!=0;handle_=INVALID_HANDLE_VALUE;
        if(!flushed||!closed)return false;
        if(!MoveFileA(FileWorkspace::Join(parent_,temp_).c_str(),
                      FileWorkspace::Join(parent_,target_).c_str()))return false;
        temp_.clear();return true;
#else
        const bool flushed=fsync(handle_)==0;
        const bool closed=close(handle_)==0;handle_=-1;
        if(!flushed||!closed)return false;
        struct stat held,actual;
        if(fstat(dir_,&held)!=0||stat(current.c_str(),&actual)!=0||
           held.st_dev!=actual.st_dev||held.st_ino!=actual.st_ino)return false;
        if(linkat(dir_,temp_.c_str(),dir_,target_.c_str(),0)!=0)return false;
        unlinkat(dir_,temp_.c_str(),0);temp_.clear();return true;
#endif
    }
    void Cancel() {
#ifdef _XBOX
        if(handle_!=INVALID_HANDLE_VALUE){CloseHandle(handle_);handle_=INVALID_HANDLE_VALUE;}
        if(!temp_.empty())DeleteFileA(FileWorkspace::Join(parent_,temp_).c_str());
#else
        if(handle_>=0){close(handle_);handle_=-1;}
        if(dir_>=0){if(!temp_.empty())unlinkat(dir_,temp_.c_str(),0);close(dir_);dir_=-1;}
#endif
        temp_.clear();parent_.clear();target_.clear();relative_.clear();
        expected_=received_=0;
    }
private:
    Upload(const Upload&);
    Upload& operator=(const Upload&);
    std::string parent_,target_,temp_,relative_;
    unsigned long long expected_,received_;
#ifdef _XBOX
    HANDLE handle_;
#else
    int dir_,handle_;
#endif
};
#endif
bool parseUploadSize(const std::string& text,unsigned long long& result,unsigned long long limit) {
    result=0;
    if(text.empty()||text.size()>9)return false;
    for(size_t i=0;i<text.size();++i) {
        if(text[i]<'0'||text[i]>'9')return false;
        result=result*10+static_cast<unsigned>(text[i]-'0');
        if(result>limit)return false;
    }
    return true;
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
    bool network,sockets,binary,closing,transferring,listing,uploadEnabled,uploading,allocated;
    unsigned long long uploadSize,restartOffset,uploadLimit;
    bool restartPending;
    Upload upload;
    FtpSession* session;
    std::string root,user,password,input,reply,payload;
    std::ifstream file;
    size_t offset;
    const FileWorkspace& workspace;
    explicit Impl(const FileWorkspace& ws):server(Invalid),control(Invalid),passive(Invalid),data(Invalid),address(0),peer(0),
        port(0),last(0),disconnected(0),network(false),sockets(false),binary(false),closing(false),
        transferring(false),listing(false),uploadEnabled(false),uploading(false),allocated(false),uploadSize(0),restartOffset(0),uploadLimit(32ULL*1024*1024),restartPending(false),session(0),offset(0),workspace(ws){}
    void clearData() {
        closeSock(passive);closeSock(data);file.close();file.clear();
        payload.clear();offset=0;transferring=false;listing=false;uploading=false;upload.Cancel();
    }
    void clearClient() {
        clearData();closeSock(control);delete session;session=0;input.clear();reply.clear();
        closing=false;binary=false;allocated=false;restartPending=false;restartOffset=0;disconnected=ticks();
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
            clearData();binary=false;allocated=false;restartPending=false;restartOffset=0;respond(session->Command(line));closing=session->Closed();return;
        }
        if(verb=="TYPE") {
            if(arg=="A"||arg=="I"){binary=arg=="I";if(!binary){restartPending=false;restartOffset=0;}respond("200 Type set");}
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
        if(verb=="REST") {
            restartPending=false;restartOffset=0;allocated=false;
            // Bound at the signed 64-bit file offset limit without overflow.
            unsigned long long n=0;
            bool ok=binary&&!arg.empty()&&arg.size()<=19;
            for(size_t i=0;ok&&i<arg.size();++i) {
                if(arg[i]<'0'||arg[i]>'9'){ok=false;break;}
                const unsigned digit=arg[i]-'0';
                if(n>(9223372036854775807ULL-digit)/10){ok=false;break;}
                n=n*10+digit;
            }
            if(!ok){respond("501 Invalid binary restart offset");return;}
            restartOffset=n;restartPending=true;
            respond("350 Restart position accepted for next RETR");return;
        }
        if(verb=="XMAX"&&arg.empty()){respond("213 "+num(uploadEnabled?uploadLimit:0));return;}
        if(verb=="ALLO"&&uploadEnabled) {
            allocated=parseUploadSize(arg,uploadSize,uploadLimit);
            respond(allocated?"200 Exact size reserved":"552 Invalid size or limit exceeded");return;
        }
        if(verb=="STOR"&&uploadEnabled) {
            const bool valid=allocated&&binary&&!restartPending;allocated=false;
            restartPending=false;restartOffset=0;
            if(!valid||passive==Invalid) {
                clearData();respond("550 Binary upload requires ALLO and passive mode");return;
            }
            const size_t slash=arg.find_last_of('/');
            std::string parent;
            if(!session->DataDirectory(slash==std::string::npos?".":slash==0?"/":arg.substr(0,slash),parent)){
                clearData();respond("550 Invalid destination");return;
            }
            const auto space=StorageInfo::Read(parent);
            if(!space.available||uploadSize>space.writableBytes){clearData();respond("452 Insufficient confirmed storage");return;}
            if(!upload.Open(*session,arg,uploadSize)) {
                clearData();respond("550 Invalid or occupied destination");return;
            }
            uploading=true;transferring=true;respond("150 Opening data connection");return;
        }
        if(verb=="SIZE") {
            std::string path;
            if(!binary||!session->DataFile(arg,path)){respond("550 File unavailable");return;}
            std::ifstream in(path.c_str(),std::ios::binary|std::ios::ate);
            if(!in||in.tellg()<0){respond("550 File unavailable");return;}
            respond("213 "+num(static_cast<unsigned long long>(in.tellg())));return;
        }
        if(verb=="LIST"||verb=="NLST"||verb=="RETR") {
            const unsigned long long start=restartPending?restartOffset:0;
            restartPending=false;restartOffset=0;
            if(passive==Invalid){respond("425 Use passive mode first");return;}
            std::string path;
            listing=verb!="RETR";
            bool ok=listing?(arg.empty()&&session->DataDirectory("",path)&&
                              directoryListing(*session,path,verb=="NLST",payload)):
                             (binary&&session->DataFile(arg,path));
            if(ok&&!listing) {
                file.open(path.c_str(),std::ios::binary|std::ios::ate);
                if(file) {
                    const std::streamoff size=file.tellg();
                    ok=size>=0&&start<=static_cast<unsigned long long>(size);
                    if(ok){file.seekg(static_cast<std::streamoff>(start),std::ios::beg);ok=!!file;}
                } else ok=false;
            }
            if(!ok){clearData();respond("550 Path unavailable or listing limit");return;}
            transferring=true;offset=0;respond("150 Opening data connection");return;
        }
        // Clear stale restart state on directory changes or unsupported commands.
        if(verb!="NOOP"&&verb!="PWD"&&verb!="XPWD"&&verb!="FEAT") {
            restartPending=false;restartOffset=0;
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
        if(uploading) {
            char buffer[32768];
            const int n=recv(data,buffer,sizeof buffer,0);
            if(n>0) {
                if(!upload.Write(buffer,static_cast<unsigned>(n)))finish(false);
                else last=ticks();
            } else if(n==0){
#ifdef __XSDK__
                finish(upload.Publish(*session,workspace));
#else
                finish(upload.Publish(*session));
#endif
            }
            else if(!pending())finish(false);
            return;
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
bool FtpServer::UploadEnabled()const{return impl_&&impl_->uploadEnabled;}
unsigned FtpServer::Port()const{return impl_?impl_->port:0;}
bool FtpServer::Start(const std::string& root,const std::string& addr,unsigned port,
                      const std::string& user,const std::string& password,bool allowUpload,unsigned long long uploadLimit) {
    Stop();error_.clear();
    if(!workspace_.CanWrite(root)||!FtpSession::ValidCredentials(user,password)||port>65535||uploadLimit==0||uploadLimit>512ULL*1024*1024) {
        error_="Workspace ou credenciais invalidos.";return false;
    }
#ifdef __XSDK__
    struct in_addr parsed={};
    if(inet_pton(AF_INET,addr.c_str(),&parsed)!=1){error_="IPv4 invalido.";return false;}
    const unsigned long ip=parsed.s_addr;
#else
    const unsigned long ip=inet_addr(addr.c_str());
#endif
    if(!localAddress(ip)){error_="IPv4 LAN privado necessario.";return false;}
    Impl* p=new Impl(workspace_);
#ifdef _XBOX
    XNetStartupParams params;std::memset(&params,0,sizeof params);
    params.cfgSizeOfStruct=sizeof params;
    if(XNetStartup(&params)!=0){delete p;error_="XNet indisponivel.";return false;}
    p->network=true;
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){delete p;error_="Winsock indisponivel.";return false;}
    p->sockets=true;
#endif
    p->address=ip;p->root=root;p->user=user;p->password=password;p->uploadEnabled=allowUpload;p->uploadLimit=uploadLimit;
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
bool FtpServer::Start(const std::string&,const std::string&,unsigned,const std::string&,const std::string&,bool,unsigned long long){
    error_="Provider FTP indisponivel nesta plataforma.";return false;
}
void FtpServer::Stop(){}
bool FtpServer::Tick(){return false;}
bool FtpServer::Running()const{return false;}
bool FtpServer::Connected()const{return false;}
bool FtpServer::UploadEnabled()const{return false;}
unsigned FtpServer::Port()const{return 0;}
#endif
