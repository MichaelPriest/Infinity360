#pragma once
#include "ClockWeather.h"
#include <set>
#define JSMN_STATIC
#define JSMN_STRICT
#include "third_party/jsmn/jsmn.h"
namespace ClockWeather {
inline bool ParseReading(const std::string& json,unsigned city,Reading& out){
    if(!city||city>=CityCount||json.empty()||json.size()>16384||json.find('\0')!=std::string::npos)return false;
    jsmntok_t tok[256];jsmn_parser p;jsmn_init(&p);int n=jsmn_parse(&p,json.data(),json.size(),tok,256);
    if(n<1||tok[0].type!=JSMN_OBJECT||json.find_first_not_of(" \r\n\t",tok[0].end)!=std::string::npos)return false;
    auto raw=[&](int i){return json.substr(tok[i].start,tok[i].end-tok[i].start);};
    auto skip=[&](int i){int end=tok[i++].end;while(i<n&&tok[i].start<end)++i;return i;};
    int current=-1;std::set<std::string> keys;
    for(int i=1;i<n;){if(tok[i].type!=JSMN_STRING||i+1>=n)return false;auto k=raw(i++);if(!keys.insert(k).second)return false;if(k=="current")current=i;i=skip(i);}
    if(current<0||tok[current].type!=JSMN_OBJECT)return false;
    Reading result;result.city=city;unsigned fields=0;keys.clear();
    for(int i=current+1;i<n&&tok[i].start<tok[current].end;){
        if(tok[i].type!=JSMN_STRING||i+1>=n)return false;auto key=raw(i++);if(!keys.insert(key).second)return false;
        if(key=="temperature_2m"||key=="weather_code"||key=="is_day"||key=="time"){
            if(tok[i].type!=JSMN_PRIMITIVE)return false;auto value=raw(i);char* end=nullptr;double number=std::strtod(value.c_str(),&end);
            if(!end||end==value.c_str()||*end||!std::isfinite(number))return false;
            if(key=="temperature_2m"){if(number< -100||number>70)return false;result.celsius=number;fields|=1;}
            else if(key=="weather_code"){if(number<0||number>99||std::floor(number)!=number||!KnownCode(static_cast<int>(number)))return false;result.code=static_cast<int>(number);fields|=2;}
            else if(key=="is_day"){if(number!=0&&number!=1)return false;result.day=number==1;fields|=4;}
            else {if(number<946684800.0||number>4102444800.0||std::floor(number)!=number)return false;result.observed=static_cast<int64_t>(number);fields|=8;}
        }
        i=skip(i);
    }
    if(fields!=15)return false;result.valid=true;out=result;return true;
}
}
