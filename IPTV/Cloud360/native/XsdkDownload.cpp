#include "XsdkDownload.h"
#include "DownloadRange.h"
#include <cctype>
#include <curl/curl.h>
#include <fstream>
#include <limits>
#include <cstring>

namespace {
struct Transfer {
    StorePackage& package;
    const std::function<bool(unsigned long long,unsigned long long)>& progress;
    unsigned long long total;
    bool cancelled=false,invalidResponse=false,rangeSeen=false,headersReady=false;
    unsigned long long offset=0;
    long response=0;
};
size_t header(char* bytes,size_t size,size_t count,void* user){
    Transfer& t=*static_cast<Transfer*>(user);
    if(size&&count>std::numeric_limits<size_t>::max()/size)return 0;
    const size_t n=size*count;
    if(n>16384)return 0;
    std::string line(bytes,n);
    if(line.compare(0,5,"HTTP/")==0){
        t.response=0;t.headersReady=false;t.rangeSeen=false;t.invalidResponse=false;
        const size_t space=line.find(' ');
        if(space==std::string::npos||space+4>line.size())return 0;
        for(size_t i=space+1;i<space+4;++i){
            if(line[i]<'0'||line[i]>'9')return 0;
            t.response=t.response*10+line[i]-'0';
        }
    }else if(line=="\r\n"||line=="\n"){
        if(t.response>=100&&t.response<200)return n;
        t.headersReady=true;
        if(t.response!=(t.offset?206:200)||(t.offset&&!t.rangeSeen))t.invalidResponse=true;
    }else{
        const size_t colon=line.find(':');
        if(colon!=std::string::npos){
            std::string name=line.substr(0,colon);
            for(char& ch:name)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            if(name=="content-range"){
                const size_t first=line.find_first_not_of(" \t",colon+1);
                const size_t last=line.find_last_not_of(" \t\r\n");
                if(t.rangeSeen||first==std::string::npos||last<first||
                   !DownloadRange::Matches(line.substr(first,last-first+1),t.offset,t.total))
                    t.invalidResponse=true;
                t.rangeSeen=true;
            }
        }
    }
    return n;
}
size_t receive(char* data,size_t size,size_t count,void* user){
    Transfer& t=*static_cast<Transfer*>(user);
    if(size&&count>std::numeric_limits<size_t>::max()/size)return 0;
    size_t length=size*count;
    if(length>32768||!t.headersReady||t.invalidResponse)return 0;
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
    SET(CURLOPT_USERAGENT,"INFINITY360-XSDK/0.1.8.1");
    SET(CURLOPT_HEADERFUNCTION,header);
    SET(CURLOPT_HEADERDATA,&t);
    SET(CURLOPT_WRITEFUNCTION,receive);
    SET(CURLOPT_WRITEDATA,&t);
    SET(CURLOPT_XFERINFOFUNCTION,status);
    SET(CURLOPT_XFERINFODATA,&t);
    SET(CURLOPT_NOPROGRESS,0L);
    #undef SET
    CURLcode result=CURLE_FAILED_INIT;
    long code=0;
    // At most two retries, within the current session. Preserve the existing
    // hash and temporary file; never append a response that ignores Range.
    for(unsigned attempt=0;options&&attempt<3;++attempt){
        t.offset=output.Done();t.headersReady=false;t.invalidResponse=false;t.rangeSeen=false;t.response=0;
        if(curl_easy_setopt(curl,CURLOPT_RESUME_FROM_LARGE,static_cast<curl_off_t>(t.offset))!=CURLE_OK){options=false;break;}
        result=curl_easy_perform(curl);
        curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&code);
        if(result==CURLE_OK||t.cancelled||t.invalidResponse||!output.Error().empty()||output.Done()>=item.bytes)break;
        if(result!=CURLE_PARTIAL_FILE&&result!=CURLE_RECV_ERROR&&result!=CURLE_OPERATION_TIMEDOUT&&result!=CURLE_SEND_ERROR)break;
        if(!progress(output.Done(),item.bytes)){t.cancelled=true;break;}
    }
    curl_easy_cleanup(curl);curl_global_cleanup();
    if(t.cancelled)return "Download cancelado ou video indisponivel.";
    if(!options)return "Configuracao HTTPS indisponivel.";
    if(result!=CURLE_OK)return output.Error().empty()?std::string("HTTPS: ")+curl_easy_strerror(result):output.Error();
    if(t.invalidResponse||!t.headersReady||code!=(t.offset?206:200))return "Resposta HTTP ou Content-Range invalido.";
    if(!progress(output.Done(),item.bytes))return "Download cancelado.";
    success=output.Finish();
    return success?(output.Error().empty()?"Pacote verificado por SHA-256 e salvo.":output.Error()):output.Error();
}
