// Linux-only TCP interoperability probe. Never linked into the Xbox XEX.
#include "FtpSession.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cctype>

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
    for(unsigned count=0;count<256 && !ftp.Closed() && line(control.fd,command);++count) {
        const size_t split=command.find(' ');
        std::string verb=command.substr(0,split);
        const std::string arg=split==std::string::npos?"":command.substr(split+1);
        for(char& ch:verb)ch=static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
        bool printable=command.size()<=512;
        for(unsigned char ch:command)if(ch<32 || ch>126)printable=false;
        if(!printable){if(!reply(control.fd,"500 Invalid command"))break;continue;}
        if(!ftp.Authenticated() || verb=="USER" || verb=="PASS" || verb=="QUIT") {
            passive.reset();
            if(!reply(control.fd,ftp.Command(command)))break;
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
            const bool valid=get?ftp.DataFile(arg,path):arg.empty()&&ftp.DataDirectory("",path);
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
