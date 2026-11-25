#include "XsdkDownload.h"
#include <curl/curl.h>
#include <fstream>
#include <limits>
#include <cstring>

namespace {
struct Transfer {
    StorePackage& package;
    const std::function<bool(unsigned long long,unsigned long long)>& progress;
    unsigned long long total;
    bool cancelled=false;
};
size_t receive(char* data,size_t size,size_t count,void* user){
    Transfer& t=*static_cast<Transfer*>(user);
    if(size&&count>std::numeric_limits<size_t>::max()/size)return 0;
    size_t length=size*count;
    if(length>32768)return 0;
    return t.package.Write(reinterpret_cast<unsigned char*>(data),static_cast<unsigned>(length))?length:0;
}
int status(void* user,curl_off_t,curl_off_t,curl_off_t,curl_off_t){
    Transfer& t=*static_cast<Transfer*>(user);
    if(!t.progress(t.package.Done(),t.total)){t.cancelled=true;return 1;}
    return 0;
}
}
std::string XsdkDownload(const StoreItem& item,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,bool& success){
    success=false;
    std::ifstream opt((root+"enable-https-store.txt").c_str());
    std::string marker;
    if(!std::getline(opt,marker)||marker!="ENABLE-HTTPS-STORE")return "Download HTTPS bloqueado: habilite a previa.";
    if(!StoreNativeSource(item,root))return "Host HTTPS nao autorizado no arquivo de servidores.";
    const std::string ca="game:/cacert.pem";
    {std::ifstream file(ca.c_str(),std::ios::binary|std::ios::ate);
     if(!file||file.tellg()<1024||file.tellg()>1024*1024)return "CA bundle ausente ou invalido.";}
    FileWorkspace workspace(root);StorePackage output(workspace);
    if(!output.Begin(item,root))return output.Error();
    if(!progress(0,item.bytes))return "Download cancelado.";
    if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)return "Falha ao inicializar HTTPS.";
    CURL* curl=curl_easy_init();
    if(!curl){curl_global_cleanup();return "Falha ao criar sessao HTTPS.";}
    Transfer t{output,progress,item.bytes};
    bool options=true;
    // Treat any unsupported security setting as fatal, rather than weaken TLS.
    #define SET(k,v) do { if(curl_easy_setopt(curl,k,v)!=CURLE_OK)options=false; } while(0)
    SET(CURLOPT_URL,item.url.c_str());
    SET(CURLOPT_PROTOCOLS_STR,"https");
    SET(CURLOPT_FOLLOWLOCATION,0L);
    SET(CURLOPT_SSL_VERIFYPEER,1L);
    SET(CURLOPT_SSL_VERIFYHOST,2L);
    SET(CURLOPT_SSLVERSION,long(CURL_SSLVERSION_TLSv1_2|CURL_SSLVERSION_MAX_TLSv1_2));
    SET(CURLOPT_CAINFO,ca.c_str());
    SET(CURLOPT_PROXY,"");
    SET(CURLOPT_NETRC,long(CURL_NETRC_IGNORED));
    SET(CURLOPT_CONNECTTIMEOUT,15L);
    SET(CURLOPT_TIMEOUT,1800L);
    SET(CURLOPT_LOW_SPEED_LIMIT,1024L);
    SET(CURLOPT_LOW_SPEED_TIME,30L);
    SET(CURLOPT_FAILONERROR,1L);
    SET(CURLOPT_MAXFILESIZE_LARGE,static_cast<curl_off_t>(item.bytes));
    SET(CURLOPT_HTTP_VERSION,long(CURL_HTTP_VERSION_1_1));
    SET(CURLOPT_USERAGENT,"INFINITY360-XSDK/0.1.8");
    SET(CURLOPT_WRITEFUNCTION,receive);
    SET(CURLOPT_WRITEDATA,&t);
    SET(CURLOPT_XFERINFOFUNCTION,status);
    SET(CURLOPT_XFERINFODATA,&t);
    SET(CURLOPT_NOPROGRESS,0L);
    #undef SET
    CURLcode result=options?curl_easy_perform(curl):CURLE_FAILED_INIT;
    long code=0;curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&code);
    curl_easy_cleanup(curl);curl_global_cleanup();
    if(t.cancelled)return "Download cancelado ou video indisponivel.";
    if(!options)return "Configuracao HTTPS indisponivel.";
    if(result!=CURLE_OK)return output.Error().empty()?std::string("HTTPS: ")+curl_easy_strerror(result):output.Error();
    if(code!=200)return "Resposta HTTP diferente de 200; redirecionamento recusado.";
    if(!progress(output.Done(),item.bytes))return "Download cancelado.";
    success=output.Finish();
    return success?(output.Error().empty()?"Pacote verificado por SHA-256 e salvo.":output.Error()):output.Error();
}
