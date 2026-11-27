#include "TvRemoteProtocol.h"
#include "TvRemoteCatalog.h"
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
    if(Parse(request("snapshot",key,host),host,key).key!=Key::Snapshot)return 13;
    if(Parse(request("snapshot","wrong",host),host,key).key!=Key::None)return 14;
    const auto page=Parse(request("tv/16/6162",key,host),host,key);
    if(page.key!=Key::TvCatalog||page.offset!=16||page.query!="ab")return 15;
    if(Parse(request("games/0/ffx",key,host),host,key).key!=Key::None)return 16;
    if(Parse(request("games/0/", "wrong",host),host,key).key!=Key::None)return 17;
    if(Quote("a\"b\\c")!="\"a\\\"b\\\\c\"")return 18;
    const auto list=CatalogPage(20,0,[](size_t){return std::string("{}");});
    if(list.find("\"next\":16")==std::string::npos)return 19;
    if(!Matches("ABC","ab"))return 20;
    if(Parse(request("tune/18446744073709551615",key,host),host,key).key!=Key::Tune)return 21;
    if(Parse(request("tune/18446744073709551616",key,host),host,key).key!=Key::None)return 22;
    if(Parse(request("tune/0",key,host),host,key).key!=Key::None)return 23;
    if(Parse(request("tune/1","wrong",host),host,key).key!=Key::None)return 24;
    return 0;
}
