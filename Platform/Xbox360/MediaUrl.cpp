#include "MediaUrl.h"

namespace {
std::string lower(std::string text) {
    for(size_t i=0; i<text.size(); ++i) if(text[i]>='A' && text[i]<='Z') text[i] += 'a'-'A';
    return text;
}
}
bool MediaUrl::Parse(const std::string& address) {
    const size_t scheme = address.find("://");
    if(scheme == std::string::npos || address.size() > 4096) return false;
    const std::string protocol = lower(address.substr(0,scheme));
    if(protocol != "http" && protocol != "https") return false;
    for(size_t i=0; i<address.size(); ++i)
        if(static_cast<unsigned char>(address[i]) <= 32 || address[i] == 127 || address[i] == '\\') return false;
    secure = protocol == "https";
    const size_t begin = scheme+3, path = address.find_first_of("/?#",begin);
    const std::string authority = address.substr(begin,path==std::string::npos ? path : path-begin);
    const size_t colon = authority.find(':');
    host = authority.substr(0,colon);
    if(host.empty() || host.size()>253 || host.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789.-")!=std::string::npos) return false;
    unsigned value=secure ? 443 : 80;
    if(colon!=std::string::npos) {
        value=0;
        for(size_t i=colon+1; i<authority.size(); ++i) {
            if(authority[i]<'0' || authority[i]>'9') return false;
            value=value*10 + (authority[i]-'0');
            if(value>65535) return false;
        }
        if(!value) return false;
    }
    port=static_cast<unsigned short>(value);
    target=path==std::string::npos ? "" : address.substr(path);
    const size_t fragment=target.find('#');
    if(fragment!=std::string::npos) target.erase(fragment);
    if(target.empty() || target[0]!='/') target.insert(0,"/");
    return true;
}
bool MediaUrl::NeedsConverter() const {
    const std::string path=lower(target.substr(0,target.find('?')));
    const size_t dot=path.find_last_of('.');
    if(dot==std::string::npos) return false;
    const std::string ext=path.substr(dot);
    const char* const formats[]={".m3u",".m3u8",".ts",".m2ts",".mp4",".m4v",".mkv",".avi",".mov",
                               ".webm",".mpd",".wmv",".asf",".mp3",".aac",".m4a",".flv"};
    for(size_t i=0;i<sizeof(formats)/sizeof(formats[0]);++i) if(ext==formats[i]) return true;
    return false;
}
