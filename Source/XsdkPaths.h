#pragma once
// Pure lexical validation for XSDK device paths; independent of host OS.
#include <string>
#include <cctype>
namespace XsdkPaths {
inline std::string Normalize(std::string path,bool allowRoot=false) {
    if(path.empty()||path.size()>240)return "";
    for(char& ch:path) {
        unsigned char b=static_cast<unsigned char>(ch);
        if(b<32||b==127||ch=='*'||ch=='?'||ch=='"'||ch=='|'||ch=='<'||ch=='>')return "";
        if(ch=='\\')ch='/';
    }
    size_t colon=path.find(':');
    if(colon==std::string::npos||path.find(':',colon+1)!=std::string::npos)return "";
    std::string drive=path.substr(0,colon);
    for(char& ch:drive)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    if(drive=="hdd1")drive="hdd";
    if(drive=="usb")drive="usb0";
    if(drive=="mu")drive="mu0";
    if(drive!="game"&&drive!="hdd"&&drive!="usb0"&&drive!="usb1"&&drive!="usb2"&&drive!="mu0"&&drive!="mu1"&&drive!="dvd")return "";
    if(colon+1<path.size()&&path[colon+1]!='/')return "";
    std::string result=drive+":/";
    size_t start=colon+1;
    while(start<path.size()){
        if(path[start]=='/'){++start;continue;}
        size_t end=path.find('/',start);
        if(end==std::string::npos)end=path.size();
        std::string part=path.substr(start,end-start);
        if(part=="."||part=="..")return "";
        if(result.back()!='/')result+='/';
        result+=part;start=end;
    }
    if(result.back()=='/'&&!allowRoot)return "";
    return result;
}
inline std::string Key(const std::string& path) {
    std::string result=Normalize(path,true);
    for(char& ch:result)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    return result;
}
inline bool Inside(const std::string& path,const std::string& root) {
    const std::string p=Key(path),r=Key(root);
    if(p.empty()||r.empty())return false;
    if(p==r)return true;
    return p.compare(0,r.size(),r)==0&&(r.back()=='/'|| (p.size()>r.size()&&p[r.size()]=='/'));
}
}
