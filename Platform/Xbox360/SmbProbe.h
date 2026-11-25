#pragma once
#include "LanPeerProbe.h"
#include <array>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <string>
#if defined(__XSDK__) || defined(__linux__)
#include <sys/select.h>
#endif

// Diagnostic only: one direct-hosted SMB2 NEGOTIATE, no authentication or share access.
// Wire fields are explicitly endian encoded for the big-endian PowerPC target.
class SmbProbe {
public:
    enum State { Idle, Connecting, Sending, Receiving, Negotiated, Rejected, Failed, Cancelled };
    struct Result { unsigned dialect=0; bool signingRequired=false; uint32_t status=0; };
    static unsigned U16(const unsigned char* b){return unsigned(b[0])|(unsigned(b[1])<<8);}
    static uint32_t U32(const unsigned char* b){return U16(b)|(uint32_t(U16(b+2))<<16);}
    static void Put16(unsigned char* b,unsigned v){b[0]=v&255;b[1]=(v>>8)&255;}
    static bool Offered(unsigned d){return d==0x0202||d==0x0210||d==0x0300||d==0x0302;}
    static const char* Dialect(unsigned d){
        switch(d){case 0x0202:return "SMB 2.0.2";case 0x0210:return "SMB 2.1";case 0x0300:return "SMB 3.0";case 0x0302:return "SMB 3.0.2";default:return "--";}
    }
    static std::array<unsigned char,112> Request(){
        std::array<unsigned char,112> b{};
        b[3]=108; // four-byte direct TCP framing, 64 + 36 + 4 dialects
        auto* p=b.data()+4;p[0]=0xfe;p[1]='S';p[2]='M';p[3]='B';
        Put16(p+4,64);Put16(p+14,1); // one requested credit, negotiate command 0
        Put16(p+64,36);Put16(p+66,4);Put16(p+68,1); // signing enabled
        // Fixed diagnostic client GUID, not a console or account identifier.
        const unsigned char guid[16]={0x49,0x4e,0x46,0x49,0x4e,0x49,0x54,0x59,0x33,0x36,0x30,0x2d,0x53,0x4d,0x42,0x32};
        std::memcpy(p+76,guid,sizeof guid);
        Put16(p+100,0x0202);Put16(p+102,0x0210);Put16(p+104,0x0300);Put16(p+106,0x0302);
        return b;
    }
    static State Parse(const unsigned char* p,size_t n,Result& result){
        result=Result{};
        if(!p||n<64||n>65536||std::memcmp(p,"\xfeSMB",4)||U16(p+4)!=64||U16(p+12)!=0||
           U32(p+16)!=1||U32(p+20)!=0||U32(p+24)!=0||U32(p+28)!=0||
           U32(p+36)!=0||U32(p+40)!=0||U32(p+44)!=0)return Failed;
        result.status=U32(p+8);
        if(result.status)return Rejected;
        if(n<128||U16(p+64)!=65)return Failed;
        const unsigned security=U16(p+66),dialect=U16(p+68);
        if(!Offered(dialect)||!(security&1)||(security&~3u))return Failed;
        const unsigned offset=U16(p+120),length=U16(p+122);
        if(length&&(offset<128||offset>n||length>n-offset))return Failed;
        result.dialect=dialect;result.signingRequired=(security&2)!=0;return Negotiated;
    }
    SmbProbe()=default;
    ~SmbProbe(){Close();}
    SmbProbe(const SmbProbe&)=delete;
    SmbProbe& operator=(const SmbProbe&)=delete;
    bool Running()const{return state_==Connecting||state_==Sending||state_==Receiving;}
    State Status()const{return state_;}
    const Result& Info()const{return info_;}
    const std::string& Label()const{return label_;}
    void Stop(){if(Running())Finish(Cancelled,"Teste SMB cancelado.");}
    bool Start(const std::string& local,const std::string& server){
        Close();info_=Result{};sent_=received_=0;expected_=4;
        if(!LanPeerProbe::Private(local)||!LanPeerProbe::Private(server)||local==server){
            Finish(Failed,"IPv4 privado distinto obrigatorio.");return false;
        }
#if defined(__XSDK__) || defined(__linux__)
        fd_=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
        if(fd_<0){Finish(Failed,"Socket TCP indisponivel.");return false;}
        const int flags=fcntl(fd_,F_GETFL,0);
        sockaddr_in src{},dst{};src.sin_family=dst.sin_family=AF_INET;dst.sin_port=htons(445);
        if(fd_>=FD_SETSIZE||flags<0||fcntl(fd_,F_SETFL,flags|O_NONBLOCK)!=0||
           inet_pton(AF_INET,local.c_str(),&src.sin_addr)!=1||inet_pton(AF_INET,server.c_str(),&dst.sin_addr)!=1||
           bind(fd_,reinterpret_cast<sockaddr*>(&src),sizeof src)!=0){
            Finish(Failed,"Falha ao preparar TCP local.");return false;
        }
        request_=Request();start_=Now();
        const int rc=connect(fd_,reinterpret_cast<sockaddr*>(&dst),sizeof dst);
        if(rc==0){state_=Sending;label_="TCP conectado; negociando SMB.";return true;}
        if(errno!=EINPROGRESS&&errno!=EWOULDBLOCK&&errno!=EAGAIN&&errno!=EINTR){
            Finish(Failed,"Conexao TCP 445 recusada ou indisponivel.");return false;
        }
        state_=Connecting;label_="Conectando TCP 445 (ate 8 s).";return true;
#else
        Finish(Failed,"Provider TCP indisponivel.");return false;
#endif
    }
    bool Tick(){
#if defined(__XSDK__) || defined(__linux__)
        if(!Running())return false;
        const auto now=Now();
        if(now<start_||now-start_>=8000){Finish(Failed,"Tempo limite SMB atingido.");return true;}
        if(state_==Connecting){
            fd_set writable;FD_ZERO(&writable);FD_SET(fd_,&writable);timeval timeout{};
            const int rc=select(fd_+1,nullptr,&writable,nullptr,&timeout);
            if(rc<0){if(errno==EINTR)return false;Finish(Failed,"Falha no teste TCP.");return true;}
            if(!rc)return false;
            int error=0;socklen_t len=sizeof error;
            if(getsockopt(fd_,SOL_SOCKET,SO_ERROR,&error,&len)!=0||error){
                Finish(Failed,"Conexao TCP 445 recusada ou indisponivel.");return true;
            }
            state_=Sending;label_="TCP conectado; negociando SMB.";return true;
        }
        if(state_==Sending){
#if defined(__linux__) && !defined(__XSDK__)
            const int flags=MSG_NOSIGNAL;
#else
            const int flags=0;
#endif
            const auto n=send(fd_,request_.data()+sent_,request_.size()-sent_,flags);
            if(n<0&&Pending())return false;
            if(n<=0){Finish(Failed,"Falha ao enviar NEGOTIATE.");return true;}
            sent_+=static_cast<size_t>(n);
            if(sent_==request_.size()){state_=Receiving;label_="Aguardando resposta NEGOTIATE.";return true;}
            return false;
        }
        const auto n=recv(fd_,response_.data()+received_,expected_-received_,0);
        if(n<0&&Pending())return false;
        if(n<=0){Finish(Failed,"Conexao encerrada sem resposta completa.");return true;}
        received_+=static_cast<size_t>(n);
        if(received_!=expected_)return false;
        if(expected_==4){
            const size_t length=(size_t(response_[1])<<16)|(size_t(response_[2])<<8)|response_[3];
            if(response_[0]||length<64||length>65536){Finish(Failed,"Quadro SMB invalido.");return true;}
            expected_=4+length;return false;
        }
        const auto result=Parse(response_.data()+4,expected_-4,info_);
        Finish(result,result==Negotiated?"Negociacao SMB confirmada; sem autenticacao.":result==Rejected?"Servidor retornou erro SMB.":"Resposta SMB invalida ou nao oferecida.");
        return true;
#else
        return false;
#endif
    }
private:
    static uint64_t Now(){return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count());}
    static bool Pending(){return errno==EAGAIN||errno==EWOULDBLOCK||errno==EINTR;}
    void Close(){
#if defined(__XSDK__) || defined(__linux__)
        if(fd_>=0){close(fd_);fd_=-1;}
#endif
    }
    void Finish(State s,const char* label){Close();state_=s;label_=label;}
    int fd_=-1;State state_=Idle;Result info_;std::string label_="Teste nao iniciado.";
    uint64_t start_=0;size_t sent_=0,received_=0,expected_=4;
    std::array<unsigned char,112> request_{};
    std::array<unsigned char,65540> response_{};
};
