#pragma once
#include <fstream>
#include <string>
#ifdef INFINITY_XSDK_HTTPS
#include <curl/curl.h>
#endif
namespace InfinityHttps {
inline bool BundlePresent(const std::string& path){
    std::ifstream in(path.c_str(),std::ios::binary|std::ios::ate);
    if(!in||in.tellg()<1024||in.tellg()>1024*1024)return false;
    const auto size=static_cast<size_t>(in.tellg());
    in.seekg(0);
    std::string bytes(size,'\0');
    if(!in.read(&bytes[0],size))return false;
    return bytes.find("-----BEGIN CERTIFICATE-----")!=std::string::npos&&bytes.find("-----END CERTIFICATE-----")!=std::string::npos;
}
struct Status {
    bool compiled=false,ssl=false,https=false,ca=false,store=false;
    std::string backend;
    std::string Label()const{
        if(!compiled)return "HTTPS: ausente nesta build";
        if(!ssl||!https)return "HTTPS: backend indisponivel";
        if(!ca)return "HTTPS: cacert.pem ausente/invalido";
        return "HTTPS: TLS + CA presentes";
    }
    static Status Read(const std::string& root){
        Status r;
#ifdef INFINITY_XSDK_HTTPS
        r.compiled=true;
        const auto* v=curl_version_info(CURLVERSION_NOW);
        if(v){
            r.ssl=(v->features&CURL_VERSION_SSL)!=0;
            if(v->ssl_version)r.backend=std::string(v->ssl_version).substr(0,80);
            if(v->protocols)for(auto p=v->protocols;*p;++p)if(std::string(*p)=="https")r.https=true;
        }
        r.ca=BundlePresent("game:/cacert.pem");
        std::ifstream flag((root+"enable-https-store.txt").c_str());
        std::string line;r.store=bool(std::getline(flag,line))&&line=="ENABLE-HTTPS-STORE";
#else
        (void)root;
#endif
        return r;
    }
};
}
