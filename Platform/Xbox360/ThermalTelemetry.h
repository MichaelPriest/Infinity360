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
    std::string Celsius(unsigned i)const{
        if(i>=4||!available[i])return "--";
        const unsigned tenths=(unsigned(raw[i])*10+128)/256;
        char value[24];std::snprintf(value,sizeof value,"%u.%u C",tenths/10,tenths%10);return value;
    }
};
