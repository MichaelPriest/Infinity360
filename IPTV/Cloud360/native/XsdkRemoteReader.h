#pragma once
#include "MpegPlayback.h"
#include "StoreHostPolicy.h"
#include "Modules/IPTV/SourcePolicy.h"
#include "xsdk/xam.h"
#include <curl/curl.h>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <vector>
namespace XsdkMpeg {
inline bool RemoteCancelled(){INPUT_STATE input{};return XamInputGetState(0,0,&input)==ERROR_SUCCESS&&(input.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK));}
// Single-threaded pull reader. Pause the libcurl transfer before the bounded buffer fills.
class RemoteReader:public MediaReader {
    CURL* easy_=nullptr;CURLM* multi_=nullptr;bool global_=false,added_=false,paused_=false,ended_=false,failed_=false;
    std::vector<unsigned char> buffer_;
    size_t read_=0;
    const char* error_="Falha ao iniciar rede IPTV.";
    static size_t Body(char* p,size_t s,size_t n,void* opaque){
        auto& self=*static_cast<RemoteReader*>(opaque);
        if(s!=1||n>32768)return 0;
        long status=0;curl_easy_getinfo(self.easy_,CURLINFO_RESPONSE_CODE,&status);
        if(status!=200)return 0;
        if(self.buffer_.size()-self.read_+n>128*1024){self.paused_=true;return CURL_WRITEFUNC_PAUSE;}
        if(self.read_){self.buffer_.erase(self.buffer_.begin(),self.buffer_.begin()+self.read_);self.read_=0;}
        self.buffer_.insert(self.buffer_.end(),p,p+n);return n;
    }
    static int Progress(void*,curl_off_t,curl_off_t,curl_off_t,curl_off_t){return RemoteCancelled()?1:0;}
public:
    const char* Error()const{return error_;}
    ~RemoteReader(){
        if(added_)curl_multi_remove_handle(multi_,easy_);
        if(easy_)curl_easy_cleanup(easy_);
        if(multi_)curl_multi_cleanup(multi_);
        if(global_)curl_global_cleanup();
    }
    bool Open(const std::string& address,const std::string& root,bool transportStream=false,bool playlist=false,bool mp4=false){
        MediaUrl url;
        error_="Endereco IPTV invalido.";
        if(!url.Parse(address)||address.find('#')!=std::string::npos)return false;
        error_="Formato remoto nao suportado.";
        if(url.NeedsConverter()){
            const auto path=StoreHostPolicy::lower(url.target.substr(0,url.target.find('?')));
            if(!(transportStream&&path.size()>=3&&path.substr(path.size()-3)==".ts")&&!(playlist&&path.size()>=5&&path.substr(path.size()-5)==".m3u8")&&!(mp4&&path.size()>=4&&path.substr(path.size()-4)==".mp4"))return false;
        }
        error_="Servidor IPTV bloqueado pelo modo restrito.";
        if(!InfinityIptv::NetworkAllowed(address,root))return false;
        if(url.secure){
            error_="Certificado cacert.pem ausente ou invalido.";
            std::ifstream ca("game:/cacert.pem",std::ios::binary|std::ios::ate);if(!ca||ca.tellg()<1024||ca.tellg()>1024*1024)return false;
        }
        error_="Falha ao inicializar libcurl IPTV.";
        if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)return false;global_=true;
        easy_=curl_easy_init();multi_=curl_multi_init();if(!easy_||!multi_)return false;
        bool ok=true;
        #define SET(k,v) do{if(curl_easy_setopt(easy_,k,v)!=CURLE_OK){ok=false;error_="Configuracao libcurl recusada: " #k;}}while(0)
        SET(CURLOPT_URL,address.c_str());SET(CURLOPT_PROTOCOLS_STR,url.secure?"https":"http");SET(CURLOPT_FOLLOWLOCATION,0L);
        if(url.secure){SET(CURLOPT_SSL_VERIFYPEER,1L);SET(CURLOPT_SSL_VERIFYHOST,2L);SET(CURLOPT_SSLVERSION,long(CURL_SSLVERSION_TLSv1_2|CURL_SSLVERSION_MAX_TLSv1_2));}
        if(url.secure){SET(CURLOPT_CAINFO,"game:/cacert.pem");}SET(CURLOPT_PROXY,"");SET(CURLOPT_NETRC,long(CURL_NETRC_IGNORED));
        SET(CURLOPT_CONNECTTIMEOUT,15L);SET(CURLOPT_LOW_SPEED_LIMIT,1024L);SET(CURLOPT_LOW_SPEED_TIME,15L);
        SET(CURLOPT_HTTP_VERSION,long(CURL_HTTP_VERSION_1_1));SET(CURLOPT_FAILONERROR,1L);
        SET(CURLOPT_WRITEFUNCTION,Body);SET(CURLOPT_WRITEDATA,this);SET(CURLOPT_XFERINFOFUNCTION,Progress);SET(CURLOPT_NOPROGRESS,0L);
        #undef SET
        if(!ok)return false;error_="Falha ao registrar transferencia IPTV.";added_=curl_multi_add_handle(multi_,easy_)==CURLM_OK;return added_;
    }
    int Read(unsigned char* data,unsigned cap) override{
        const auto start=std::chrono::steady_clock::now();
        while(read_==buffer_.size()&&!ended_&&!failed_){
            if(RemoteCancelled()||std::chrono::steady_clock::now()-start>std::chrono::seconds(15))return -1;
            if(paused_){paused_=false;if(curl_easy_pause(easy_,CURLPAUSE_CONT)!=CURLE_OK)return -1;}
            int running=0;if(curl_multi_perform(multi_,&running)!=CURLM_OK)return -1;
            int left=0;while(auto* msg=curl_multi_info_read(multi_,&left)){
                if(msg->msg==CURLMSG_DONE){ended_=true;failed_=msg->data.result!=CURLE_OK;}
            }
            if(read_==buffer_.size()&&!ended_){int activity=0;if(curl_multi_poll(multi_,nullptr,0,50,&activity)!=CURLM_OK)return -1;}
        }
        if(RemoteCancelled()||failed_)return -1;
        const size_t n=std::min<size_t>(cap,buffer_.size()-read_);
        if(n)std::memcpy(data,buffer_.data()+read_,n);read_+=n;return static_cast<int>(n);
    }
};
}
