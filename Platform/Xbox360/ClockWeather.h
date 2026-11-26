#pragma once
#include <string>
#include <ctime>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cmath>
namespace ClockWeather {
constexpr int Auto=10000,MinOffset=-720,MaxOffset=840,Step=15;
struct City{const char* name;const char* latitude;const char* longitude;};
inline constexpr City Cities[]={
    {"Desativado","",""},{"Sao Paulo","-23.55","-46.63"},{"Rio de Janeiro","-22.91","-43.17"},{"Brasilia","-15.79","-47.88"},
    {"Belo Horizonte","-19.92","-43.94"},{"Curitiba","-25.43","-49.27"},{"Porto Alegre","-30.03","-51.23"},
    {"Salvador","-12.97","-38.50"},{"Recife","-8.05","-34.88"},{"Fortaleza","-3.73","-38.52"},{"Manaus","-3.10","-60.02"},
    {"Belem","-1.46","-48.50"},{"Goiania","-16.68","-49.25"},{"Lisboa","38.72","-9.14"},{"Londres","51.51","-0.13"},
    {"Nova York","40.71","-74.01"},{"Buenos Aires","-34.60","-58.38"}
};
constexpr unsigned CityCount=sizeof(Cities)/sizeof(Cities[0]);
inline bool ValidOffset(int n){return n==Auto||(n>=MinOffset&&n<=MaxOffset&&n%Step==0);}
inline bool Int(const std::string& text,int& value){
    if(text.empty()||text.size()>6)return false;
    size_t i=text[0]=='-'?1:0;if(i==text.size())return false;int n=0;
    for(;i<text.size();++i){if(text[i]<'0'||text[i]>'9')return false;n=n*10+text[i]-'0';}
    value=text[0]=='-'?-n:n;return true;
}
struct Settings{
    int offset=Auto;unsigned city=0;
    std::string Encode()const{return std::to_string(offset)+":"+std::to_string(city);}
    static bool Parse(const std::string& text,Settings& out){
        auto colon=text.find(':');int offset,city;
        if(colon==std::string::npos||!Int(text.substr(0,colon),offset)||!Int(text.substr(colon+1),city)||!ValidOffset(offset)||city<0||static_cast<unsigned>(city)>=CityCount)return false;
        out={offset,static_cast<unsigned>(city)};return true;
    }
};
inline int NextOffset(int offset,int direction){
    if(offset==Auto)return direction<0?MaxOffset:MinOffset;
    int n=offset+(direction<0?-Step:Step);return n<MinOffset||n>MaxOffset?Auto:n;
}
inline std::string Zone(int offset){
    if(offset==Auto)return "Sistema";
    if(!ValidOffset(offset))return "UTC ?";
    char s[20];unsigned n=static_cast<unsigned>(std::abs(offset));
    std::snprintf(s,sizeof s,"UTC%c%02u:%02u",offset<0?'-':'+',n/60,n%60);return s;
}
inline std::string Format(int64_t epoch,int offset,const char* fmt,const char* fallback){
    if(epoch<946684800LL||epoch>4102444800LL||!ValidOffset(offset))return fallback;
    if(offset!=Auto)epoch+=static_cast<int64_t>(offset)*60;
    const auto time=static_cast<std::time_t>(epoch);
    if(static_cast<int64_t>(time)!=epoch)return fallback;
    const auto* tm=offset==Auto?std::localtime(&time):std::gmtime(&time);char buffer[40]{};
    return tm&&std::strftime(buffer,sizeof buffer,fmt,tm)?buffer:fallback;
}
inline std::string Time(int64_t epoch,int offset){return Format(epoch,offset,"%H:%M","--:--");}
inline std::string Date(int64_t epoch,int offset){return Format(epoch,offset,"%d/%m/%Y","--/--/----");}
inline std::string Url(unsigned city){
    if(!city||city>=CityCount)return "";
    return std::string("https://api.open-meteo.com/v1/forecast?latitude=")+Cities[city].latitude+"&longitude="+Cities[city].longitude+"&current=temperature_2m,weather_code,is_day&timeformat=unixtime&timezone=GMT";
}
inline bool KnownCode(int code){
    switch(code){case 0:case 1:case 2:case 3:case 45:case 48:case 51:case 53:case 55:case 56:case 57:case 61:case 63:case 65:case 66:case 67:case 71:case 73:case 75:case 77:case 80:case 81:case 82:case 85:case 86:case 95:case 96:case 99:return true;default:return false;}
}
struct Reading{
    bool valid=false;unsigned city=0;double celsius=0;int code=0;bool day=true;int64_t observed=0;
    bool Fresh(int64_t now)const{return valid&&now>=946684800LL&&observed<=now+900&&now-observed<=7200;}
    std::string Temperature()const{char b[20];std::snprintf(b,sizeof b,"%.0f C",celsius);return b;}
    const char* Condition()const{
        if(code==0)return day?"Céu limpo":"Noite limpa";
        if(code<=2)return "Parcial nublado";if(code==3)return "Nublado";if(code<=48)return "Neblina";
        if(code>=95)return "Trovoadas";if((code>=71&&code<=77)||code==85||code==86)return "Neve";
        return "Chuva";
    }
};
}
