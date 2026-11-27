#pragma once
#include "XsdkRemoteReader.h"
#include "http_range.h"
namespace XsdkMpeg {
// Bounded synchronous HTTP range cache. No body is published until its
// complete response, identity and exact Content-Range have been validated.
class RangeReader{
    enum { Window=256*1024 };
    CURL* easy_=nullptr;bool global_=false;
    std::string address_,tag_,responseTag_,rangeText_,encoding_;
    std::vector<unsigned char> cache_,pending_;
    uint64_t length_=0,pos_=0,start_=0;
    bool bad_=false;
    static size_t Header(char* p,size_t s,size_t n,void* u){
        auto& r=*static_cast<RangeReader*>(u);
        if(s!=1||n>8192)return 0;
        std::string line(p,n);
        if(line.compare(0,5,"HTTP/")==0){r.responseTag_.clear();r.rangeText_.clear();r.encoding_.clear();r.bad_=false;return n;}
        auto colon=line.find(':');if(colon==std::string::npos)return n;
        auto key=StoreHostPolicy::lower(line.substr(0,colon));
        auto first=line.find_first_not_of(" \t",colon+1),last=line.find_last_not_of(" \t\r\n");
        std::string value=first==std::string::npos||last<first?"":line.substr(first,last-first+1);
        std::string* dest=key=="etag"?&r.responseTag_:key=="content-range"?&r.rangeText_:key=="content-encoding"?&r.encoding_:nullptr;
        if(dest){if(!dest->empty()||value.empty())r.bad_=true;*dest=value;}
        return n;
    }
    static size_t Body(char* p,size_t s,size_t n,void* u){
        auto& r=*static_cast<RangeReader*>(u);
        long code=0;curl_easy_getinfo(r.easy_,CURLINFO_RESPONSE_CODE,&code);
        if(s!=1||code!=206||n>Window-r.pending_.size())return 0;
        r.pending_.insert(r.pending_.end(),p,p+n);return n;
    }
    static int Progress(void*,curl_off_t,curl_off_t,curl_off_t,curl_off_t){return RemoteCancelled()?1:0;}
    bool Fetch(uint64_t first){
        const auto last=first+Window-1;
        const auto range=std::to_string(first)+"-"+std::to_string(last);
        for(unsigned attempt=0;attempt<2&&!RemoteCancelled();++attempt){
            pending_.clear();responseTag_.clear();rangeText_.clear();encoding_.clear();bad_=false;
            curl_slist* headers=nullptr;
            if(!tag_.empty()){headers=curl_slist_append(nullptr,("If-Range: "+tag_).c_str());if(!headers)return false;}
            bool ok=curl_easy_setopt(easy_,CURLOPT_RANGE,range.c_str())==CURLE_OK&&
                curl_easy_setopt(easy_,CURLOPT_HTTPHEADER,headers)==CURLE_OK;
            const auto rc=ok?curl_easy_perform(easy_):CURLE_FAILED_INIT;
            long code=0;curl_easy_getinfo(easy_,CURLINFO_RESPONSE_CODE,&code);
            curl_easy_setopt(easy_,CURLOPT_HTTPHEADER,nullptr);curl_slist_free_all(headers);
            if(rc!=CURLE_OK)continue;
            CloudRange::Range parsed;
            if(code!=206||bad_||(!encoding_.empty()&&StoreHostPolicy::lower(encoding_)!="identity")||
                !CloudRange::Parse(rangeText_,parsed)||!CloudRange::Matches(parsed,first,last,length_)||
                !CloudRange::StrongTag(responseTag_)||(!tag_.empty()&&tag_!=responseTag_)||
                pending_.size()!=parsed.last-parsed.first+1)return false;
            length_=parsed.total;tag_=responseTag_;start_=first;cache_.swap(pending_);pending_.clear();return true;
        }
        return false;
    }
public:
    RangeReader()=default;
    RangeReader(const RangeReader&)=delete;
    RangeReader& operator=(const RangeReader&)=delete;
    ~RangeReader(){if(easy_)curl_easy_cleanup(easy_);if(global_)curl_global_cleanup();}
    bool Open(const std::string& address,const std::string& root){
        MediaUrl url;
        if(easy_||!url.Parse(address)||address.find('#')!=std::string::npos||
           !InfinityIptv::NetworkAllowed(address,root))return false;
        if(url.secure){std::ifstream ca("game:/cacert.pem",std::ios::binary|std::ios::ate);if(!ca||ca.tellg()<1024||ca.tellg()>1024*1024)return false;}
        if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)return false;global_=true;
        easy_=curl_easy_init();if(!easy_)return false;
        address_=address;bool ok=true;
        #define RANGE_SET(k,v) do{if(curl_easy_setopt(easy_,k,v)!=CURLE_OK)ok=false;}while(0)
        RANGE_SET(CURLOPT_URL,address_.c_str());RANGE_SET(CURLOPT_PROTOCOLS_STR,url.secure?"https":"http");
        RANGE_SET(CURLOPT_FOLLOWLOCATION,0L);RANGE_SET(CURLOPT_SSL_VERIFYPEER,1L);RANGE_SET(CURLOPT_SSL_VERIFYHOST,2L);
        RANGE_SET(CURLOPT_SSLVERSION,long(CURL_SSLVERSION_TLSv1_2|CURL_SSLVERSION_MAX_TLSv1_2));
        RANGE_SET(CURLOPT_CAINFO,"game:/cacert.pem");RANGE_SET(CURLOPT_PROXY,"");RANGE_SET(CURLOPT_NETRC,long(CURL_NETRC_IGNORED));
        RANGE_SET(CURLOPT_CONNECTTIMEOUT,15L);RANGE_SET(CURLOPT_TIMEOUT,30L);
        RANGE_SET(CURLOPT_HTTP_VERSION,long(CURL_HTTP_VERSION_1_1));RANGE_SET(CURLOPT_FAILONERROR,1L);
        RANGE_SET(CURLOPT_ACCEPT_ENCODING,"identity");RANGE_SET(CURLOPT_HTTP_CONTENT_DECODING,0L);
        RANGE_SET(CURLOPT_HEADERFUNCTION,Header);RANGE_SET(CURLOPT_HEADERDATA,this);
        RANGE_SET(CURLOPT_WRITEFUNCTION,Body);RANGE_SET(CURLOPT_WRITEDATA,this);
        RANGE_SET(CURLOPT_XFERINFOFUNCTION,Progress);RANGE_SET(CURLOPT_NOPROGRESS,0L);
        #undef RANGE_SET
        cache_.reserve(Window);pending_.reserve(Window);
        return ok&&Fetch(0);
    }
    int64_t Length()const{return static_cast<int64_t>(length_);}
    static int64_t SeekCallback(void* u,int64_t p){
        auto& r=*static_cast<RangeReader*>(u);
        if(p<0||uint64_t(p)>r.length_||!r.length_||RemoteCancelled())return -1;
        r.pos_=static_cast<uint64_t>(p);return p;
    }
    static int ReadCallback(void* u,uint8_t* b,unsigned n){
        auto& r=*static_cast<RangeReader*>(u);
        if(!b||!n||RemoteCancelled()||!r.length_)return -1;
        if(r.pos_==r.length_)return 0;
        if(r.pos_<r.start_||r.pos_-r.start_>=r.cache_.size())
            if(!r.Fetch(r.pos_))return -1;
        const auto offset=static_cast<size_t>(r.pos_-r.start_);
        const auto count=std::min<size_t>(n,r.cache_.size()-offset);
        std::memcpy(b,r.cache_.data()+offset,count);r.pos_+=count;return static_cast<int>(count);
    }
};
}
