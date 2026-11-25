#pragma once
#include <cstddef>
#include <cstdint>
#include <string>
#include <cstdio>

#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMC_READ)
// xboxkrnl export 41 (0x029); XSDK import added only in experimental build.
extern "C" void HalSendSMCMessage(void* input,void* output);
#endif
struct ThermalTelemetry {
    enum State { Unsupported, NotRead, Invalid, Partial, Valid };
    State state=NotRead;
    uint16_t raw[4]={};
    bool available[4]={};
    static bool Supported(){
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMC_READ)
        return true;
#else
        return false;
#endif
    }
    // Free60 command 0x07; four little-endian unsigned 8.8 Celsius values.
    // Reject zero/unreasonable values independently; do not invent a reading.
    static ThermalTelemetry Decode(const unsigned char* b,size_t n){
        ThermalTelemetry t;t.state=Invalid;
        if(!b||n!=16||b[0]!=0x07)return t;
        unsigned good=0;
        for(unsigned i=0;i<4;++i){
            const unsigned value=unsigned(b[1+i*2])|(unsigned(b[2+i*2])<<8);
            if(value>=256&&value<=125u*256u){
                t.raw[i]=static_cast<uint16_t>(value);t.available[i]=true;++good;
            }
        }
        t.state=good==4?Valid:good?Partial:Invalid;return t;
    }
    static ThermalTelemetry Read(){
#if defined(__XSDK__) && defined(INFINITY_EXPERIMENTAL_SMC_READ)
        // Only read command; never change thermal targets, fan or power.
        unsigned char request[16]={0x07},response[16]={};
        HalSendSMCMessage(request,response);
        return Decode(response,sizeof response);
#else
        ThermalTelemetry t;t.state=Unsupported;return t;
#endif
    }
    static std::string FormatCelsius(uint16_t rawValue){
        const unsigned tenths=(unsigned(rawValue)*10+128)/256;
        char value[24];std::snprintf(value,sizeof value,"%u.%u C",tenths/10,tenths%10);return value;
    }
    std::string Celsius(unsigned i)const{
        return i<4&&available[i]?FormatCelsius(raw[i]):"--";
    }
};

// Session-only manual observations; invalid samples never become extrema.
struct ThermalHistory {
    uint16_t minimum[4]={},maximum[4]={};
    bool seen[4]={};
    uint32_t attempts=0,valid=0,partial=0,invalid=0;
    static void Increment(uint32_t& n){if(n<UINT32_MAX)++n;}
    void Add(const ThermalTelemetry& sample){
        if(sample.state!=ThermalTelemetry::Valid&&sample.state!=ThermalTelemetry::Partial&&sample.state!=ThermalTelemetry::Invalid)return;
        Increment(attempts);
        if(sample.state==ThermalTelemetry::Invalid){Increment(invalid);return;}
        if(sample.state==ThermalTelemetry::Valid)Increment(valid);else Increment(partial);
        for(unsigned i=0;i<4;++i){
            if(!sample.available[i]||sample.raw[i]<256||sample.raw[i]>125u*256u)continue;
            const uint16_t v=sample.raw[i];
            if(!seen[i]){minimum[i]=maximum[i]=v;seen[i]=true;}
            else{if(v<minimum[i])minimum[i]=v;if(v>maximum[i])maximum[i]=v;}
        }
    }
    void Clear(){*this=ThermalHistory();}
    std::string Min(unsigned i)const{return i<4&&seen[i]?ThermalTelemetry::FormatCelsius(minimum[i]):"--";}
    std::string Max(unsigned i)const{return i<4&&seen[i]?ThermalTelemetry::FormatCelsius(maximum[i]):"--";}
};
