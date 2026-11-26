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
    int current=-1,daily=-1,offset=0;bool offsetValid=false;std::set<std::string> keys;
    for(int i=1;i<n;){if(tok[i].type!=JSMN_STRING||i+1>=n)return false;auto k=raw(i++);if(!keys.insert(k).second)return false;if(k=="current")current=i;else if(k=="daily")daily=i;else if(k=="utc_offset_seconds"){int z=0;if(tok[i].type==JSMN_PRIMITIVE&&Int(raw(i),z)&&z>=-43200&&z<=50400&&z%60==0){offset=z;offsetValid=true;}}i=skip(i);}
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
    if(fields!=15)return false;
    // Optional daily arrays are accepted only as a complete, consistent forecast.
    if(daily>=0&&tok[daily].type==JSMN_OBJECT&&offsetValid){
        const char* names[]={"time","temperature_2m_min","temperature_2m_max","weather_code","precipitation_probability_max"};
        double values[5][5]{};unsigned counts[5]{};bool good=true;std::set<std::string> dailyKeys;
        for(int i=daily+1;i<n&&tok[i].start<tok[daily].end;){
            if(tok[i].type!=JSMN_STRING||i+1>=n){good=false;break;}
            auto key=raw(i++);if(!dailyKeys.insert(key).second){good=false;break;}
            int field=-1;for(int k=0;k<5;++k)if(key==names[k])field=k;
            if(field>=0){
                if(tok[i].type!=JSMN_ARRAY){good=false;break;}
                for(int z=i+1;z<n&&tok[z].start<tok[i].end;++z){
                    if(counts[field]>=5||tok[z].type!=JSMN_PRIMITIVE){good=false;break;}
                    auto text=raw(z);char* end=nullptr;double x=std::strtod(text.c_str(),&end);
                    if(field==4&&text=="null")x=-1;
                    else if(!end||end==text.c_str()||*end||!std::isfinite(x)){good=false;break;}
                    values[field][counts[field]++]=x;
                }
            }
            i=skip(i);
        }
        unsigned count=counts[0];
        if(!count||count>5)good=false;
        for(int k=1;k<5;++k)if(counts[k]!=count)good=false;
        for(unsigned k=0;good&&k<count;++k){
            const double time=values[0][k],lo=values[1][k],hi=values[2][k],code=values[3][k],rain=values[4][k];
            if(time<946684800.0||time>4102444800.0||std::floor(time)!=time||(k&&time<=values[0][k-1])||
               lo< -100||hi>70||lo>hi||code<0||code>99||std::floor(code)!=code||!KnownCode(static_cast<int>(code))||
               rain< -1||rain>100||std::floor(rain)!=rain){good=false;break;}
            result.days[k]={static_cast<int64_t>(time),lo,hi,static_cast<int>(code),static_cast<int>(rain)};
        }
        if(good){result.dayCount=count;result.cityOffsetSeconds=offset;}
    }
    result.valid=true;out=result;return true;
}
}
