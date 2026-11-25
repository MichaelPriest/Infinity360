#pragma once
#include <string>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <chrono>
#if defined(__XSDK__) || defined(__linux__)
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#endif

// Explicit two-peer LAN diagnostic, not game traffic or Xbox Live emulation.
class LanPeerProbe {
public:
    static constexpr unsigned Port=32160;
    ~LanPeerProbe(){Stop();}
    LanPeerProbe()=default;
    LanPeerProbe(const LanPeerProbe&)=delete;
    LanPeerProbe& operator=(const LanPeerProbe&)=delete;
    static bool Private(const std::string& ip){
        unsigned b[4]={};size_t pos=0;
        for(unsigned i=0;i<4;++i){
            size_t start=pos;
            while(pos<ip.size()&&ip[pos]>='0'&&ip[pos]<='9'){
                if(pos-start>=3)return false;
                b[i]=b[i]*10+ip[pos++]-'0';
            }
            if(pos==start||b[i]>255||(pos-start>1&&ip[start]=='0'))return false;
            if(i!=3&&(pos>=ip.size()||ip[pos++]!='.'))return false;
        }
        if(pos!=ip.size()||b[3]==0||b[3]==255)return false;
        return b[0]==10||(b[0]==172&&b[1]>=16&&b[1]<=31)||(b[0]==192&&b[1]==168);
    }
    // Fixed 12-byte message: magic, version, kind, reserved, opaque nonce.
    static void Packet(unsigned char* b,bool response,uint32_t nonce){
        const unsigned char prefix[]={ 'I','N','F','L',1,static_cast<unsigned char>(response?2:1),0,0 };
        std::memcpy(b,prefix,8);for(unsigned i=0;i<4;++i)b[8+i]=(nonce>>(24-8*i))&255;
    }
    static bool Decode(const unsigned char* b,size_t n,bool& response,uint32_t& nonce){
        if(!b||n!=12||std::memcmp(b,"INFL",4)||b[4]!=1||(b[5]!=1&&b[5]!=2)||b[6]||b[7])return false;
        response=b[5]==2;nonce=0;for(unsigned i=8;i<12;++i)nonce=(nonce<<8)|b[i];return nonce!=0;
    }
    bool Start(const std::string& local,const std::string& peer){
        Stop();sent_=received_=0;rtt_=0;lastAccepted_=0;peerText_=peer;
        if(!Private(local)||!Private(peer)||local==peer){label_="IPv4 privado distinto obrigatorio.";return false;}
#if defined(__XSDK__) || defined(__linux__)
        fd_=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
        if(fd_<0){label_="Falha ao abrir UDP.";return false;}
        sockaddr_in address{};address.sin_family=AF_INET;address.sin_port=htons(Port);
        peer_={};peer_.sin_family=AF_INET;peer_.sin_port=htons(Port);
        const int flags=fcntl(fd_,F_GETFL,0);
        if(flags<0||fcntl(fd_,F_SETFL,flags|O_NONBLOCK)!=0||
           inet_pton(AF_INET,local.c_str(),&address.sin_addr)!=1||
           inet_pton(AF_INET,peer.c_str(),&peer_.sin_addr)!=1||
           bind(fd_,reinterpret_cast<sockaddr*>(&address),sizeof address)!=0){
            Stop();label_="Falha ao vincular UDP local.";return false;
        }
        start_=Now();lastSend_=start_-2000;lastReply_=start_-1000;nonce_=static_cast<uint32_t>(start_)|1u;
        label_="Teste UDP em andamento (15 s).";return true;
#else
        label_="Provider UDP indisponivel.";return false;
#endif
    }
    bool Tick(){
#if defined(__XSDK__) || defined(__linux__)
        if(fd_<0)return false;
        const auto now=Now();bool changed=false;
        if(now-start_>=15000){Stop();label_=received_?"UDP bidirecional confirmado.":"Sem resposta UDP no intervalo.";return true;}
        if(sent_<5&&now-lastSend_>=2000){
            ++nonce_;if(!nonce_)++nonce_;
            unsigned char b[12];Packet(b,false,nonce_);lastSend_=now;
            if(sendto(fd_,b,sizeof b,0,reinterpret_cast<sockaddr*>(&peer_),sizeof peer_)==sizeof b){++sent_;changed=true;}
        }
        for(unsigned i=0;i<4;++i){
            unsigned char b[64];sockaddr_in from{};socklen_t len=sizeof from;
            const auto n=recvfrom(fd_,b,sizeof b,0,reinterpret_cast<sockaddr*>(&from),&len);
            if(n<0)break;
            bool reply=false;uint32_t nonce=0;
            if(from.sin_family!=AF_INET||from.sin_addr.s_addr!=peer_.sin_addr.s_addr||
               from.sin_port!=peer_.sin_port||!Decode(b,static_cast<size_t>(n),reply,nonce))continue;
            if(!reply&&now-lastReply_>=500){
                Packet(b,true,nonce);sendto(fd_,b,12,0,reinterpret_cast<sockaddr*>(&peer_),sizeof peer_);lastReply_=now;
            }else if(reply&&nonce==nonce_&&lastAccepted_!=nonce&&sent_){
                lastAccepted_=nonce;++received_;rtt_=now-lastSend_;changed=true;
            }
        }
        return changed;
#else
        return false;
#endif
    }
    void Stop(){
#if defined(__XSDK__) || defined(__linux__)
        if(fd_>=0){close(fd_);fd_=-1;label_="Teste encerrado.";}
#endif
    }
    bool Running()const{return fd_>=0;}
    unsigned Sent()const{return sent_;}
    unsigned Received()const{return received_;}
    uint64_t Rtt()const{return rtt_;}
    const std::string& Label()const{return label_;}
    const std::string& Peer()const{return peerText_;}
private:
    static uint64_t Now(){return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
    int fd_=-1;unsigned sent_=0,received_=0;uint32_t nonce_=0,lastAccepted_=0;
    uint64_t start_=0,lastSend_=0,lastReply_=0,rtt_=0;
    std::string label_="Teste nao iniciado.",peerText_;
#if defined(__XSDK__) || defined(__linux__)
    sockaddr_in peer_{};
#endif
};
