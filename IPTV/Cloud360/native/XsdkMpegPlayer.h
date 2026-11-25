#pragma once
#include "MpegPlayback.h"
#include "HomeCanvas.h"
#include "MediaUrl.h"
#include "XsdkPaths.h"
#include "xsdk/audio.h"
#include "xsdk/xam.h"
#include <fstream>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <functional>
#ifdef INFINITY_XSDK_HTTPS
#include <curl/curl.h>
#include "StorePackage.h"
#endif
namespace XsdkMpeg {
inline bool Cancelled(){
    INPUT_STATE input{};return XamInputGetState(0,0,&input)==ERROR_SUCCESS&&(input.gamepad.buttons&(INPUT_GAMEPAD_B|INPUT_GAMEPAD_BACK));
}
class LocalReader:public MediaReader{
    std::ifstream file_;
public:
    explicit LocalReader(const std::string& path):file_(path.c_str(),std::ios::binary){}
    bool Ready() const{return bool(file_);}
    int Read(unsigned char* data,unsigned capacity) override{
        if(Cancelled())return -1;
        file_.read(reinterpret_cast<char*>(data),capacity);
        if(file_.bad())return -1;
        return static_cast<int>(file_.gcount());
    }
};
#ifdef INFINITY_XSDK_HTTPS
// Single-threaded pull reader. Pause the libcurl transfer before the bounded buffer fills.
class RemoteReader:public MediaReader {
    CURL* easy_=nullptr;CURLM* multi_=nullptr;bool global_=false,added_=false,paused_=false,ended_=false,failed_=false;
    std::vector<unsigned char> buffer_;
    size_t read_=0;
    static size_t Body(char* p,size_t s,size_t n,void* opaque){
        auto& self=*static_cast<RemoteReader*>(opaque);
        if(s!=1||n>32768)return 0;
        long status=0;curl_easy_getinfo(self.easy_,CURLINFO_RESPONSE_CODE,&status);
        if(status!=200)return 0;
        if(self.buffer_.size()-self.read_+n>128*1024){self.paused_=true;return CURL_WRITEFUNC_PAUSE;}
        if(self.read_){self.buffer_.erase(self.buffer_.begin(),self.buffer_.begin()+self.read_);self.read_=0;}
        self.buffer_.insert(self.buffer_.end(),p,p+n);return n;
    }
    static int Progress(void*,curl_off_t,curl_off_t,curl_off_t,curl_off_t){return Cancelled()?1:0;}
public:
    ~RemoteReader(){
        if(added_)curl_multi_remove_handle(multi_,easy_);
        if(easy_)curl_easy_cleanup(easy_);
        if(multi_)curl_multi_cleanup(multi_);
        if(global_)curl_global_cleanup();
    }
    bool Open(const std::string& address,const std::string& root){
        MediaUrl url;StoreItem source{};source.url=address;
        if(!url.Parse(address)||url.NeedsConverter()||address.find('#')!=std::string::npos)return false;
        // HTTP only to an explicitly configured LAN relay address. HTTPS uses the existing exact host allowlist.
        if(!url.secure){
            std::ifstream relay((root+"infinity360-iptv-relay.txt").c_str());std::string allowed;
            if(!std::getline(relay,allowed)||allowed!=url.host||url.host.find_first_not_of("0123456789.")!=std::string::npos)return false;
            unsigned a,b,c,d;char tail;
            if(std::sscanf(url.host.c_str(),"%u.%u.%u.%u%c",&a,&b,&c,&d,&tail)!=4||a>255||b>255||c>255||d>255||
               !(a==10||(a==172&&b>=16&&b<=31)||(a==192&&b==168)))return false;
            if(url.host!=std::to_string(a)+"."+std::to_string(b)+"."+std::to_string(c)+"."+std::to_string(d))return false;
        }else{
            if(!StoreNativeSource(source,root))return false;
            std::ifstream ca("game:/cacert.pem",std::ios::binary|std::ios::ate);if(!ca||ca.tellg()<1024||ca.tellg()>1024*1024)return false;
        }
        if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)return false;global_=true;
        easy_=curl_easy_init();multi_=curl_multi_init();if(!easy_||!multi_)return false;
        bool ok=true;
        #define SET(k,v) do{if(curl_easy_setopt(easy_,k,v)!=CURLE_OK)ok=false;}while(0)
        SET(CURLOPT_URL,address.c_str());SET(CURLOPT_PROTOCOLS_STR,url.secure?"https":"http");SET(CURLOPT_FOLLOWLOCATION,0L);
        SET(CURLOPT_SSL_VERIFYPEER,1L);SET(CURLOPT_SSL_VERIFYHOST,2L);SET(CURLOPT_SSLVERSION,long(CURL_SSLVERSION_TLSv1_2|CURL_SSLVERSION_MAX_TLSv1_2));
        SET(CURLOPT_CAINFO,"game:/cacert.pem");SET(CURLOPT_PROXY,"");SET(CURLOPT_NETRC,long(CURL_NETRC_IGNORED));
        SET(CURLOPT_CONNECTTIMEOUT,15L);SET(CURLOPT_LOW_SPEED_LIMIT,1024L);SET(CURLOPT_LOW_SPEED_TIME,15L);
        SET(CURLOPT_HTTP_VERSION,long(CURL_HTTP_VERSION_1_1));SET(CURLOPT_FAILONERROR,1L);
        SET(CURLOPT_WRITEFUNCTION,Body);SET(CURLOPT_WRITEDATA,this);SET(CURLOPT_XFERINFOFUNCTION,Progress);SET(CURLOPT_NOPROGRESS,0L);
        #undef SET
        if(!ok)return false;added_=curl_multi_add_handle(multi_,easy_)==CURLM_OK;return added_;
    }
    int Read(unsigned char* data,unsigned cap) override{
        const auto start=std::chrono::steady_clock::now();
        while(read_==buffer_.size()&&!ended_&&!failed_){
            if(Cancelled()||std::chrono::steady_clock::now()-start>std::chrono::seconds(15))return -1;
            if(paused_){paused_=false;if(curl_easy_pause(easy_,CURLPAUSE_CONT)!=CURLE_OK)return -1;}
            int running=0;if(curl_multi_perform(multi_,&running)!=CURLM_OK)return -1;
            int left=0;while(auto* msg=curl_multi_info_read(multi_,&left)){
                if(msg->msg==CURLMSG_DONE){ended_=true;failed_=msg->data.result!=CURLE_OK;}
            }
            if(read_==buffer_.size()&&!ended_){int activity=0;if(curl_multi_poll(multi_,nullptr,0,50,&activity)!=CURLM_OK)return -1;}
        }
        if(Cancelled()||failed_)return -1;
        const size_t n=std::min<size_t>(cap,buffer_.size()-read_);
        if(n)std::memcpy(data,buffer_.data()+read_,n);read_+=n;return static_cast<int>(n);
    }
};
#endif
class Output:public MpegOutput{
    HomeCanvas& canvas_;
    std::vector<unsigned char> rgb_;
    float pcm_[2][256]{};
    unsigned count_=0,phase_=0,rate_=0;
    bool audio_=false,muted_=false;
    double aspect_=1.0;
public:
    bool dirty=false;
    explicit Output(HomeCanvas& c):canvas_(c){}
    ~Output(){if(audio_)xsdk_audio_close();}
    void SetAspect(double aspect){aspect_=aspect;}
    void SetMuted(bool muted){muted_=muted;}
    bool Start(int rate){
        if(!rate)return true;
        if(rate<8000||rate>48000)return false;
        rate_=static_cast<unsigned>(rate);audio_=NT_SUCCESS(xsdk_audio_open(16));return audio_;
    }
    bool Video(plm_frame_t& frame) override{
        if(Cancelled()||!frame.width||!frame.height||frame.width>1280||frame.height>720)return false;
        rgb_.resize(frame.width*frame.height*3);
        plm_frame_to_rgb(&frame,rgb_.data(),frame.width*3);
        canvas_.VideoRgb(rgb_.data(),frame.width,frame.height,aspect_);dirty=true;return true;
    }
    bool Audio(const plm_samples_t& samples) override{
        if(!audio_||!rate_)return false;
        for(unsigned i=0;i<samples.count;++i){
            phase_+=48000;
            while(phase_>=rate_){
                phase_-=rate_;
                for(unsigned ch=0;ch<2;++ch)pcm_[ch][count_]=muted_?0.0f:std::max(-1.0f,std::min(1.0f,samples.interleaved[i*2+ch]));
                if(++count_==256){
                    if(Cancelled())return false;
                    float* frame=nullptr;if(!NT_SUCCESS(xsdk_audio_next_frame(250,&frame))||!frame)return false;
                    std::fill(frame,frame+XSDK_AUDIO_FRAME_FLOATS,0.0f);
                    std::copy(pcm_[0],pcm_[0]+256,frame);std::copy(pcm_[1],pcm_[1]+256,frame+256);
                    xsdk_audio_queue_frame();count_=0;
                }
            }
        }
        return true;
    }
};
inline std::string PlayReader(MediaReader& reader,HomeCanvas& canvas,const std::function<bool(bool)>& present,bool local=false){
    Output output(canvas);MpegPlayback player(reader,output);
    if(!player.Open())return player.Error();
    output.SetAspect(player.PixelAspect());
    if(!output.Start(player.SampleRate()))return "Audio XSDK indisponivel.";
    using Clock=std::chrono::steady_clock;
    auto last=Clock::now();double elapsed=0;
    bool paused=false,muted=false;
    INPUT_STATE initial{};unsigned previous=XamInputGetState(0,0,&initial)==ERROR_SUCCESS?initial.gamepad.buttons:0;
    while(!Cancelled()){
        INPUT_STATE input{};
        if(XamInputGetState(0,0,&input)==ERROR_SUCCESS){
            const unsigned buttons=input.gamepad.buttons,edge=buttons&~previous;previous=buttons;
            if(edge&INPUT_GAMEPAD_X){muted=!muted;output.SetMuted(muted);}
            if(local&&(edge&INPUT_GAMEPAD_A)){paused=!paused;last=Clock::now();}
        }
        if(paused){
            canvas.Box(40,620,520,58,0xff172c28u,6);
            canvas.Text(56,633,"Pausado | A Retomar | B Sair",23,0xfff2f7f3u,490);
            if(!present(true))return "Falha GPU.";
            last=Clock::now();continue;
        }
        auto now=Clock::now();const double delta=std::chrono::duration<double>(now-last).count();
        if(delta<0.025){if(!present(false))return "Falha GPU.";continue;}
        last=now;elapsed=std::min(0.1,delta);
        const bool running=player.Advance(elapsed);
        if(!present(output.dirty))return "Falha GPU.";
        output.dirty=false;
        if(!running)return player.Error().empty()?"Reproducao encerrada.":player.Error();
    }
    return "Reproducao cancelada.";
}
inline std::string Play(const std::string& path,const std::string& root,HomeCanvas& canvas,const std::function<bool(bool)>& present){
    std::ifstream marker((root+"enable-iptv-playback.txt").c_str());std::string enabled;
    if(!std::getline(marker,enabled)||enabled!="ENABLE-IPTV-PLAYBACK")return "Habilite enable-iptv-playback.txt.";
    MediaUrl url;
    if(url.Parse(path)){
#ifdef INFINITY_XSDK_HTTPS
        RemoteReader reader;if(!reader.Open(path,root))return "Fonte MPEG-PS nao autorizada ou indisponivel.";
        return PlayReader(reader,canvas,present);
#else
        return "Transporte de rede indisponivel.";
#endif
    }
    const auto local=XsdkPaths::Normalize(path);
    if(local.empty()||(!tvEndsWith(local,".mpg")&&!tvEndsWith(local,".mpeg")&&!tvEndsWith(local,".ps")))return "Use MPEG-1/MP2 em MPEG-PS; WMV nao suportado neste XSDK.";
    LocalReader reader(local);if(!reader.Ready())return "Arquivo local indisponivel.";
    return PlayReader(reader,canvas,present,true);
}
}
