#include "UnityCatalog.h"
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
            ++at;while(at<n&&tokens[at].start<value.end)++at;
        }
        if(!item.id||!ids.insert(item.id).second)return false;
        result.push_back(item);
    }
    if(at!=n||result.size()!=static_cast<size_t>(tokens[0].size))return false;
    updates.swap(result);return true;
}
