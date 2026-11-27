#pragma once
#include "TvRemoteProtocol.h"
#include "LanPeerProbe.h"
#include "SavedFile.h"
#include "NetworkInfo.h"
#include <cerrno>
namespace TvRemote {
class Server {
public:
    static constexpr unsigned Port=32161;
    ~Server(){Stop();}
    Server()=default;Server(const Server&)=delete;Server& operator=(const Server&)=delete;
    bool Start(const std::string& root){
        Stop();
        std::ifstream in(savedFile(root+"cloud360-remote.txt").c_str(),std::ios::binary);
        char ip[32]{},key[40]{};
        if(!in.getline(ip,sizeof ip)||!in.getline(key,sizeof key)||in.peek()!=std::char_traits<char>::eof())return false;
        std::string peer=ip,secret=key;
        if(!peer.empty()&&peer.back()=='\r')peer.pop_back();
        if(!secret.empty()&&secret.back()=='\r')secret.pop_back();
        if(!LanPeerProbe::Private(peer)||!ValidSecret(secret))return false;
        const auto net=NetworkInfo::Read();
        if(net.state!=NetworkSnapshot::AddressReady||!LanPeerProbe::Private(net.ipv4)||net.ipv4==peer)return false;
#if defined(__XSDK__) || defined(__linux__)
        sockaddr_in a{};a.sin_family=AF_INET;a.sin_port=htons(Port);
        if(inet_pton(AF_INET,net.ipv4.c_str(),&a.sin_addr)!=1||inet_pton(AF_INET,peer.c_str(),&peer_)!=1)return false;
        listener_=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(listener_<0||!Nonblock(listener_)||bind(listener_,reinterpret_cast<sockaddr*>(&a),sizeof a)!=0||listen(listener_,1)!=0){Stop();return false;}
        host_=net.ipv4+":"+std::to_string(Port);secret_=secret;return true;
#else
        return false;
#endif
    }
    std::string Address()const{return listener_>=0?"http://"+host_:"";}
    Key Poll(){
#if defined(__XSDK__) || defined(__linux__)
        if(listener_<0)return Key::None;
        const auto now=Now();
        if(client_>=0&&now>deadline_)CloseClient();
        if(client_<0){
            sockaddr_in from{};socklen_t len=sizeof from;
            int fd=accept(listener_,reinterpret_cast<sockaddr*>(&from),&len);
            if(fd<0)return Key::None;
            if(from.sin_family!=AF_INET||from.sin_addr.s_addr!=peer_.s_addr||!Nonblock(fd)){close(fd);return Key::None;}
            client_=fd;deadline_=now+2000;
        }
        Key result=Key::None;
        if(response_.empty()){
            char b[512];const auto count=recv(client_,b,sizeof b,0);
            if(count==0){CloseClient();return Key::None;}
            if(count<0){if(errno!=EAGAIN&&errno!=EWOULDBLOCK)CloseClient();return Key::None;}
            request_.append(b,static_cast<size_t>(count));
            if(request_.size()>2048){CloseClient();return Key::None;}
            if(request_.find("\r\n\r\n")==std::string::npos)return Key::None;
            const auto parsed=Parse(request_,host_,secret_);
            const bool allowed=parsed.key!=Key::None&&(!lastCommand_||now-lastCommand_>=150);
            if(allowed){result=parsed.key;lastCommand_=now;}
            const std::string body=parsed.page?Page():allowed?"OK":"Recusado";
            response_=std::string("HTTP/1.1 ")+(parsed.page||allowed?"200 OK":"403 Forbidden")+"\r\nConnection: close\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nContent-Type: "+(parsed.page?"text/html; charset=utf-8":"text/plain; charset=utf-8")+"\r\nContent-Length: "+std::to_string(body.size())+"\r\n\r\n"+body;
        }
        const auto left=response_.size()-sent_;
#ifdef __XSDK__
        const auto count=send(client_,response_.data()+sent_,left,0);
#else
        const auto count=send(client_,response_.data()+sent_,left,MSG_NOSIGNAL);
#endif
        if(count>0){sent_+=static_cast<size_t>(count);if(sent_==response_.size())CloseClient();}
        else if(count==0||(errno!=EAGAIN&&errno!=EWOULDBLOCK))CloseClient();
        return result;
#else
        return Key::None;
#endif
    }
    void Stop(){
#if defined(__XSDK__) || defined(__linux__)
        CloseClient();if(listener_>=0)close(listener_);
#endif
        listener_=-1;host_.clear();secret_.clear();lastCommand_=0;
    }
private:
    int listener_=-1,client_=-1;std::string host_,secret_,request_,response_;size_t sent_=0;
    uint64_t deadline_=0,lastCommand_=0;
    static uint64_t Now(){return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
#if defined(__XSDK__) || defined(__linux__)
    in_addr peer_{};
    static bool Nonblock(int fd){const int flags=fcntl(fd,F_GETFL,0);return flags>=0&&fcntl(fd,F_SETFL,flags|O_NONBLOCK)==0;}
    void CloseClient(){if(client_>=0)close(client_);client_=-1;request_.clear();response_.clear();sent_=0;}
#endif
};
}
