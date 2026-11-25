#include "UnityCatalog.h"
#include "MediaUrl.h"
#include <algorithm>
#include <cctype>
#define JSMN_STATIC
#define JSMN_STRICT
#include "third_party/jsmn/jsmn.h"
#include <cstdio>
#include <set>
namespace {
bool decimal(const std::string& value,unsigned long long max,unsigned long long& out){
    out=0;if(value.empty()||value.size()>12)return false;
    for(char ch:value){if(ch<'0'||ch>'9'||out>(max-static_cast<unsigned>(ch-'0'))/10)return false;out=out*10+ch-'0';}
    return out>0;
}
std::string raw(const std::string& data,const jsmntok_t& t){
    return data.substr(static_cast<size_t>(t.start),static_cast<size_t>(t.end-t.start));
}
bool scalar(const jsmntok_t& t){return t.type==JSMN_STRING||t.type==JSMN_PRIMITIVE;}
}
std::string UnityCatalog::FileUrl(const std::string& input){
    std::string url=input;
    if(url.compare(0,7,"http://")==0)url.replace(0,7,"https://");
    MediaUrl parsed;
    if(url.compare(0,8,"https://")!=0||url.size()>1024||url.find('#')!=std::string::npos||
       !parsed.Parse(url)||!parsed.secure||parsed.port!=443)return "";
    std::string host=parsed.host;
    for(char& ch:host)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if(host!="xboxunity.net"&&host!="www.xboxunity.net")return "";
    return url;
}
std::string UnityCatalog::Md5Url(unsigned id){
    return id?"https://xboxunity.net/api/tumd5/"+std::to_string(id):"";
}
bool UnityCatalog::ParseMd5(const std::string& data,std::string& digest){
    digest.clear();if(data.empty()||data.size()>4096||data.find('\0')!=std::string::npos)return false;
    jsmntok_t tokens[64];jsmn_parser parser;jsmn_init(&parser);
    const int n=jsmn_parse(&parser,data.data(),data.size(),tokens,64);
    if(n<1||tokens[0].type!=JSMN_OBJECT||data.find_first_not_of(" \r\n\t",tokens[0].end)!=std::string::npos)return false;
    std::set<std::string> keys;std::string found;
    for(int i=1;i<n;){
        if(tokens[i].type!=JSMN_STRING||i+1>=n)return false;
        const auto key=raw(data,tokens[i++]);const auto value=tokens[i++];
        if(!keys.insert(key).second)return false;
        if(key=="md5"){
            if(value.type!=JSMN_STRING)return false;
            found=raw(data,value);
            if(found.size()!=32||found.find_first_not_of("0123456789abcdefABCDEF")!=std::string::npos)return false;
            for(char& c:found)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        while(i<n&&tokens[i].start<value.end)++i;
    }
    if(found.empty())return false;digest=found;return true;
}
std::string UnityCatalog::UpdatesUrl(const XboxExecutionInfo& info){
    if(!info.titleId||!info.baseVersion)return "";
    char url[96];std::sprintf(url,"https://xboxunity.net/api/tu/%08X/%08X",info.titleId,info.baseVersion);
    return url;
}
bool UnityCatalog::ParseUpdates(const std::string& data,std::vector<UnityUpdate>& updates){
    updates.clear();
    if(data.empty()||data.size()>256*1024||data.find('\0')!=std::string::npos)return false;
    for(unsigned char ch:data)if(ch<32&&ch!='\t'&&ch!='\n'&&ch!='\r')return false;
    std::vector<jsmntok_t> tokens(16384);jsmn_parser parser;jsmn_init(&parser);
    const int n=jsmn_parse(&parser,data.data(),data.size(),tokens.data(),tokens.size());
    if(n<1||tokens[0].type!=JSMN_ARRAY||tokens[0].size>256)return false;
    if(data.find_first_not_of(" \t\r\n",tokens[0].end)!=std::string::npos)return false;
    std::vector<UnityUpdate> result;std::set<unsigned> ids;
    int at=1;
    while(at<n&&tokens[at].start<tokens[0].end){
        const auto row=tokens[at++];
        if(row.type!=JSMN_OBJECT||row.size>64)return false;
        UnityUpdate item;std::set<std::string> keys;
        while(at<n&&tokens[at].start<row.end){
            const auto key=tokens[at++];
            if(key.type!=JSMN_STRING||at>=n||tokens[at].start>=row.end)return false;
            const std::string field=raw(data,key);
            if(!keys.insert(field).second)return false;
            const auto value=tokens[at];
            if(field=="TitleUpdateID"||field=="filesize"||field=="version"){
                if(!scalar(value))return false;
                const auto s=raw(data,value);
                unsigned long long number=0;
                if(field=="TitleUpdateID"){
                    if(!decimal(s,0xffffffffULL,number))return false;
                    item.id=static_cast<unsigned>(number);
                }else if(field=="filesize"){
                    if(decimal(s,128ULL*1024*1024,number))item.bytes=number;
                }else{
                    if(s.size()<=24&&!s.empty()&&s.find_first_not_of("0123456789abcdefABCDEFxX.- ")==std::string::npos)item.version=s;
                }
            }
            if(field=="filename"&&value.type==JSMN_STRING){
                const auto name=raw(data,value);
                if(!name.empty()&&name.size()<=42&&name.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-.")==std::string::npos&&name!="."&&name!="..")item.filename=name;
            }
            if(field=="url"&&value.type==JSMN_STRING){
                auto address=raw(data,value);
                // Decode JSON escaped slashes only; other escapes are rejected by URL parser.
                size_t pos=0;while((pos=address.find("\\/",pos))!=std::string::npos)address.erase(pos,1);
                item.url=FileUrl(address);
            }
            ++at;while(at<n&&tokens[at].start<value.end)++at;
        }
        if(!item.id||!ids.insert(item.id).second)return false;
        result.push_back(item);
    }
    if(at!=n||result.size()!=static_cast<size_t>(tokens[0].size))return false;
    updates.swap(result);return true;
}

std::string UnityCatalog::CoversUrl(unsigned title){
    if(!title)return "";char url[72];std::sprintf(url,"https://xboxunity.net/api/Covers/%08X",title);return url;
}
std::string UnityCatalog::CoverUrl(unsigned image){
    return image?"https://xboxunity.net/api/boxartfront/"+std::to_string(image):"";
}
bool UnityCatalog::ParseCovers(const std::string& data,unsigned title,std::vector<unsigned>& images){
    images.clear();if(!title||data.empty()||data.size()>256*1024||data.find('\0')!=std::string::npos)return false;
    std::vector<jsmntok_t> tokens(16384);jsmn_parser parser;jsmn_init(&parser);
    const int n=jsmn_parse(&parser,data.data(),data.size(),tokens.data(),tokens.size());
    if(n<1||tokens[0].type!=JSMN_ARRAY||tokens[0].size>256||
       data.find_first_not_of(" \t\r\n",tokens[0].end)!=std::string::npos)return false;
    std::vector<unsigned> result;std::set<unsigned> ids;
    int at=1;size_t count=0;
    while(at<n&&tokens[at].start<tokens[0].end){
        const auto row=tokens[at++];if(row.type!=JSMN_OBJECT||row.size>64)return false;++count;
        unsigned long long tid=0,image=0;std::set<std::string> keys;
        while(at<n&&tokens[at].start<row.end){
            const auto key=tokens[at++];if(key.type!=JSMN_STRING||at>=n)return false;
            const auto field=raw(data,key);if(!keys.insert(field).second)return false;
            const auto value=tokens[at++];
            if(field=="titleid"||field=="imageid"){
                if(!scalar(value)||!decimal(raw(data,value),0xffffffffULL,field=="titleid"?tid:image))return false;
            }
            while(at<n&&tokens[at].start<value.end)++at;
        }
        if(tid==title&&image&&ids.insert(static_cast<unsigned>(image)).second)result.push_back(static_cast<unsigned>(image));
    }
    if(at!=n||count!=static_cast<size_t>(tokens[0].size))return false;
    images.swap(result);return true;
}
