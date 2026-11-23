#include "XboxHttpStream.h"
#include <winsockx.h>
#include <cstdio>
#include <cstring>

XboxHttpStream::XboxHttpStream(bool (*cancel)()) : cancel_(cancel),net_(false),sockets_(false),http_(false),
    cancelled_(false),session_(0),connection_(0),request_(0),status_(0),received_(0),buffer_(32768) {}
XboxHttpStream::~XboxHttpStream() {
    if(request_) XHttpCloseHandle(request_);
    if(connection_) XHttpCloseHandle(connection_);
    if(session_) XHttpCloseHandle(session_);
    if(http_) XHttpShutdown();
    if(sockets_) WSACleanup();
    if(net_) XNetCleanup();
}
void CALLBACK XboxHttpStream::OnStatus(HINTERNET,DWORD_PTR context,DWORD status,LPVOID,DWORD length) {
    XboxHttpStream* self=reinterpret_cast<XboxHttpStream*>(context);
    if(!self) return;
    switch(status) {
    case XHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE:
    case XHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
    case XHTTP_CALLBACK_STATUS_REQUEST_ERROR:
    case XHTTP_CALLBACK_STATUS_READ_COMPLETE:
        self->status_=status;
        self->received_=length;
        break;
    }
}
bool XboxHttpStream::Interrupted(DWORD since) {
    if(cancel_()) { cancelled_=true;return true; }
    if(GetTickCount()-since<15000) return false;
    error_="O servidor demorou para responder.";return true;
}
bool XboxHttpStream::Wait(DWORD wanted) {
    const DWORD since=GetTickCount();
    while(status_!=wanted) {
        if(Interrupted(since)) return false;
        if(status_==XHTTP_CALLBACK_STATUS_REQUEST_ERROR || !XHttpDoWork(session_,0)) {
            error_="Falha na conexão com a fonte de vídeo.";return false;
        }
        if(status_!=wanted) Sleep(2);
    }
    return true;
}
bool XboxHttpStream::Open(const MediaUrl& url) {
    error_="Não foi possível iniciar a rede.";
    XNetStartupParams params;
    ZeroMemory(&params,sizeof params);params.cfgSizeOfStruct=sizeof params;
    params.cfgFlags=XNET_STARTUP_BYPASS_SECURITY; // include ordinary LAN peers
    if(XNetStartup(&params)!=0) return false;
    net_=true;
    WSADATA wsa;
    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0) return false;
    sockets_=true;
    XNADDR address;
    const DWORD since=GetTickCount();
    while(XNetGetTitleXnAddr(&address)==XNET_GET_XNADDR_PENDING) {
        if(Interrupted(since)) return false;
        Sleep(10);
    }
    if(!XHttpStartup(0,0)) return false;
    http_=true;
    session_=XHttpOpen("Cloud360/1",XHTTP_ACCESS_TYPE_DEFAULT_PROXY,0,0,XHTTP_FLAG_ASYNC);
    if(!session_) return false;
    error_="Não foi possível abrir a transmissão.";
    connection_=XHttpConnect(session_,url.host.c_str(),url.port,url.secure?XHTTP_FLAG_SECURE:0);
    if(!connection_) return false;
    request_=XHttpOpenRequest(connection_,"GET",url.target.c_str(),0,0,0,0);
    if(!request_) return false;
    XHttpSetStatusCallback(request_,OnStatus,XHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS,0);
    if(!XHttpSendRequest(request_,0,0,0,0,0,reinterpret_cast<DWORD_PTR>(this)) ||
       !Wait(XHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE)) return false;
    status_=0;
    if(!XHttpReceiveResponse(request_,0) || !Wait(XHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE)) return false;
    DWORD code=0, size=sizeof code;
    if(!XHttpQueryHeaders(request_,XHTTP_QUERY_STATUS_CODE|XHTTP_QUERY_FLAG_NUMBER,
                          XHTTP_HEADER_NAME_BY_INDEX,&code,&size,XHTTP_NO_HEADER_INDEX)) return false;
    if(code!=200) {
        char message[96];std::sprintf(message,"O servidor respondeu com HTTP %lu.",static_cast<unsigned long>(code));
        error_=message;return false;
    }
    error_.clear();
    return true;
}
int XboxHttpStream::Read(unsigned char* data,unsigned capacity) {
    if(cancel_()) {cancelled_=true;return -1;}
    const DWORD want=capacity < buffer_.size() ? capacity : static_cast<DWORD>(buffer_.size());
    status_=0;
    if(!XHttpReadData(request_,&buffer_[0],want,0) || !Wait(XHTTP_CALLBACK_STATUS_READ_COMPLETE) || received_>want)
        return -1;
    if(received_) std::memcpy(data,&buffer_[0],received_);
    return static_cast<int>(received_);
}
