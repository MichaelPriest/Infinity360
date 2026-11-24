// Linux-only TCP interoperability probe. Never linked into the Xbox XEX.
#include "FtpSession.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cctype>
#include <fcntl.h>
#include <cerrno>
#include <sys/stat.h>

namespace fs=std::filesystem;
namespace {
struct Socket {
    int fd;
    explicit Socket(int value=-1):fd(value){}
    ~Socket(){ reset(); }
    void reset(int value=-1){if(fd>=0)close(fd);fd=value;}
    Socket(const Socket&)=delete;
    Socket& operator=(const Socket&)=delete;
};
void timeout(int fd) {
    const timeval tv={5,0};
    setsockopt(fd,SOL_SOCKET,SO_RCVTIMEO,&tv,sizeof(tv));
    setsockopt(fd,SOL_SOCKET,SO_SNDTIMEO,&tv,sizeof(tv));
}
bool sendAll(int fd,const char* p,size_t n) {
    while(n) {
        const ssize_t count=send(fd,p,n,MSG_NOSIGNAL);
        if(count<=0)return false;
        p+=count;n-=static_cast<size_t>(count);
    }
    return true;
}
bool reply(int fd,const std::string& msg) {
    const std::string line=msg+"\r\n";
    return sendAll(fd,line.data(),line.size());
}
bool line(int fd,std::string& out) {
    out.clear();
    for(size_t i=0;i<514;++i) {
        char ch;
        if(recv(fd,&ch,1,0)!=1)return false;
        if(ch=='\n') {
            if(out.empty() || out.back()!='\r')return false;
            out.pop_back();return true;
        }
        out+=ch;
    }
    return false;
}
unsigned listenLocal(Socket& s) {
    s.reset(socket(AF_INET,SOCK_STREAM,0));
    if(s.fd<0)return 0;
    sockaddr_in address={};
    address.sin_family=AF_INET;
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(bind(s.fd,reinterpret_cast<sockaddr*>(&address),sizeof(address)) ||
       listen(s.fd,1)) {s.reset();return 0;}
    socklen_t size=sizeof(address);
    if(getsockname(s.fd,reinterpret_cast<sockaddr*>(&address),&size)) {s.reset();return 0;}
    timeout(s.fd);
    return ntohs(address.sin_port);
}
int acceptLocal(Socket& s) {
    sockaddr_in peer={};
    socklen_t len=sizeof(peer);
    int fd=accept(s.fd,reinterpret_cast<sockaddr*>(&peer),&len);
    if(fd<0)return -1;
    if(peer.sin_addr.s_addr!=htonl(INADDR_LOOPBACK)){close(fd);return -1;}
    timeout(fd);return fd;
}
bool listFiles(FtpSession& ftp,int fd,const std::string& path,bool namesOnly) {
    std::error_code ec;
    fs::directory_iterator it(path,ec),end;
    if(ec)return false;
    size_t count=0;
    for(;it!=end;it.increment(ec)) {
        if(ec || ++count>4096)return false;
        const std::string name=it->path().filename().string();
        std::string validated;
        if(!FileWorkspace::ValidName(name))continue;
        const bool dir=ftp.DataDirectory(name,validated);
        if(!dir && !ftp.DataFile(name,validated))continue;
        std::string entry=name+"\r\n";
        if(!namesOnly) {
            const auto size=dir?0:fs::file_size(validated,ec);
            if(ec)return false;
            entry=std::string(dir?"dr-xr-xr-x":"-r--r--r--")+" 1 owner owner "+
                std::to_string(size)+" Jan 01 2000 "+entry;
        }
        if(!sendAll(fd,entry.data(),entry.size()))return false;
    }
    return !ec;
}
// STOR is experimental: require ALLO exact size, cap at 32 MiB.
const unsigned long long MaxUpload=32ull*1024*1024;
bool parseSize(const std::string& text,unsigned long long& size) {
    size=0;
    if(text.empty() || text.size()>8)return false;
    for(char ch:text) {
        if(ch<'0' || ch>'9')return false;
        size=size*10+static_cast<unsigned>(ch-'0');
        if(size>MaxUpload)return false;
    }
    return true;
}
bool uploadTarget(FtpSession& ftp,const std::string& arg,std::string& parent,std::string& name) {
    if(arg.empty() || arg.size()>200)return false;
    const size_t slash=arg.find_last_of('/');
    name=slash==std::string::npos?arg:arg.substr(slash+1);
    return FileWorkspace::ValidName(name) &&
        ftp.DataDirectory(slash==std::string::npos?".":slash==0?"/":arg.substr(0,slash),parent);
}
bool receiveFile(int fd,const std::string& parent,const std::string& name,unsigned long long expected) {
    Socket dir(open(parent.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW));
    if(dir.fd<0)return false;
    struct stat info={};
    if(fstatat(dir.fd,name.c_str(),&info,AT_SYMLINK_NOFOLLOW)==0 || errno!=ENOENT)return false;
    Socket output;
    std::string temp;
    for(unsigned i=0;i<32;++i) {
        const std::string candidate="AXCOPY-FTP-"+std::to_string(i)+".part";
        output.reset(openat(dir.fd,candidate.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600));
        if(output.fd>=0){temp=candidate;break;}
    }
    if(output.fd<0)return false;
    bool ok=true;
    unsigned long long received=0;
    char buffer[65536];
    while(ok) {
        const ssize_t n=recv(fd,buffer,sizeof(buffer),0);
        if(n==0)break;
        if(n<0 && errno==EINTR)continue;
        if(n<0 || static_cast<unsigned long long>(n)>expected-received){ok=false;break;}
        size_t done=0;
        while(done<static_cast<size_t>(n)) {
            const ssize_t w=write(output.fd,buffer+done,static_cast<size_t>(n)-done);
            if(w<0 && errno==EINTR)continue;
            if(w<=0){ok=false;break;}
            done+=static_cast<size_t>(w);
        }
        received+=done;
    }
    ok=ok && received==expected && fsync(output.fd)==0;
    if(close(output.fd)!=0)ok=false;
    output.fd=-1;
    // Publish only a complete upload; linkat never replaces an existing name.
    if(ok)ok=linkat(dir.fd,temp.c_str(),dir.fd,name.c_str(),0)==0;
    unlinkat(dir.fd,temp.c_str(),0);
    return ok;
}
bool retrieve(int fd,const std::string& path) {
    std::ifstream in(path,std::ios::binary);
    if(!in)return false;
    char buffer[65536];
    while(in) {
        in.read(buffer,sizeof(buffer));
        const auto n=in.gcount();
        if(n && !sendAll(fd,buffer,static_cast<size_t>(n)))return false;
    }
    return in.eof() && !in.bad();
}
}
int main(int argc,char** argv) {
    if(argc!=2)return 2;
    std::string pass;
    if(!std::getline(std::cin,pass))return 2;
    FileWorkspace workspace(argv[1]);
    FtpSession ftp(workspace,FileWorkspace::Join(argv[1],"UserFiles"),"owner",pass);
    if(ftp.Closed())return 2;
    Socket listener,control,passive;
    unsigned port=listenLocal(listener);
    if(!port)return 3;
    std::cout<<port<<std::endl;
    control.reset(acceptLocal(listener));
    listener.reset();
    if(control.fd<0 || !reply(control.fd,ftp.Greeting()))return 3;
    std::string command;
    bool binary=false, allocated=false;
    unsigned long long uploadSize=0;
    for(unsigned count=0;count<256 && !ftp.Closed() && line(control.fd,command);++count) {
        const size_t split=command.find(' ');
        std::string verb=command.substr(0,split);
        const std::string arg=split==std::string::npos?"":command.substr(split+1);
        for(char& ch:verb)ch=static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        bool printable=command.size()<=512;
        for(unsigned char ch:command)if(ch<32 || ch>126)printable=false;
        if(!printable){if(!reply(control.fd,"500 Invalid command"))break;continue;}
        if(!ftp.Authenticated() || verb=="USER" || verb=="PASS" || verb=="QUIT") {
            passive.reset();binary=false;allocated=false;
            if(!reply(control.fd,ftp.Command(command)))break;
            continue;
        }
        if(verb=="TYPE") {
            if(arg=="A" || arg=="I") {
                binary=arg=="I";
                if(!reply(control.fd,"200 Type set"))break;
            } else if(!reply(control.fd,"504 Unsupported type"))break;
            continue;
        }
        if(verb=="PASV" || verb=="EPSV") {
            const unsigned p=listenLocal(passive);
            const std::string response=!p?"425 Cannot open data listener":
                verb=="EPSV"?"229 Entering Extended Passive Mode (|||"+std::to_string(p)+"|)":
                "227 Entering Passive Mode (127,0,0,1,"+std::to_string(p/256)+","+std::to_string(p%256)+")";
            if(!reply(control.fd,response))break;
            continue;
        }
        if(verb=="ALLO") {
            allocated=parseSize(arg,uploadSize);
            if(!reply(control.fd,allocated?"200 Exact size reserved":"552 Invalid size or limit exceeded"))break;
            continue;
        }
        if(verb=="STOR") {
            std::string parent,name;
            const bool valid=allocated && binary && uploadTarget(ftp,arg,parent,name);
            allocated=false;
            if(!valid || passive.fd<0) {
                passive.reset();
                if(!reply(control.fd,valid?"425 Use passive mode first":"550 Binary upload requires ALLO and valid path"))break;
                continue;
            }
            if(!reply(control.fd,"150 Opening data connection"))break;
            Socket data(acceptLocal(passive));passive.reset();
            const bool ok=data.fd>=0 && receiveFile(data.fd,parent,name,uploadSize);
            data.reset();
            if(!reply(control.fd,ok?"226 Transfer complete":"426 Upload rejected or incomplete"))break;
            continue;
        }
        if(verb=="SIZE") {
            std::string path;std::error_code ec;
            const bool valid=ftp.DataFile(arg,path);
            const auto size=valid?fs::file_size(path,ec):0;
            if(!reply(control.fd,valid&&!ec?"213 "+std::to_string(size):"550 File unavailable"))break;
            continue;
        }
        if(verb=="LIST" || verb=="NLST" || verb=="RETR") {
            // This probe lists the current directory only, with no LIST options.
            std::string path;
            const bool get=verb=="RETR";
            const bool valid=get?(binary&&ftp.DataFile(arg,path)):arg.empty()&&ftp.DataDirectory("",path);
            if(!valid || passive.fd<0) {
                passive.reset();
                if(!reply(control.fd,valid?"425 Use passive mode first":"550 Path unavailable"))break;
                continue;
            }
            if(!reply(control.fd,"150 Opening data connection"))break;
            Socket data(acceptLocal(passive));passive.reset();
            const bool ok=data.fd>=0 && (get?retrieve(data.fd,path):
                listFiles(ftp,data.fd,path,verb=="NLST"));
            data.reset();
            if(!reply(control.fd,ok?"226 Transfer complete":"426 Transfer failed"))break;
            continue;
        }
        if(!reply(control.fd,ftp.Command(command)))break;
    }
    return 0;
}
