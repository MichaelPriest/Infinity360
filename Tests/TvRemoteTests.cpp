#include "TvRemoteProtocol.h"
int main(){
    using namespace TvRemote;
    const std::string host="192.168.1.4:32161",key="0123456789abcdef0123456789abcdef";
    const auto request=[&](const std::string& command,const std::string& token,const std::string& h){return "POST /cmd/"+command+" HTTP/1.1\r\nHost: "+h+"\r\nX-Infinity-Key: "+token+"\r\nContent-Length: 0\r\n\r\n";};
    if(!ValidSecret(key)||ValidSecret("1234"))return 1;
    if(!Parse("GET / HTTP/1.1\r\nHost: "+host+"\r\n\r\n",host,key).page)return 2;
    if(Parse(request("quality",key,host),host,key).key!=Key::Quality)return 3;
    if(Parse(request("quality","wrong",host),host,key).key!=Key::None)return 4;
    if(Parse(request("quality",key,"other"),host,key).key!=Key::None)return 5;
    if(Parse(request("launch",key,host),host,key).key!=Key::None)return 6;
    if(Parse(request("mute",key,host)+"body",host,key).key!=Key::None)return 7;
    if(Parse(request("mute",key,host),host,key).key!=Key::Mute)return 8;
    if(Parse(request("status",key,host),host,key).key!=Key::Status)return 9;
    if(Parse(request("guide",key,host),host,key).key!=Key::Guide)return 10;
    if(Parse(request("favorite",key,host),host,key).key!=Key::Favorite)return 11;
    if(Parse(request("power",key,host),host,key).key!=Key::None)return 12;
    return 0;
}
