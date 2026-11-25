#include "XsdkDownload.h"
#include "UnityStage.h"
#include "ArtImport.h"
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
    if(!output.Begin(item,root,true))return output.Error();
    if(!progress(output.Done(),item.bytes))return output.Pause()?"Parcial salvo. START retoma.":output.Error();
    if(output.Done()==item.bytes){success=output.Finish();return success?"Pacote recuperado e verificado.":output.Error();}
    if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK){output.Pause();return "Falha ao inicializar HTTPS.";}
    CURL* curl=curl_easy_init();
    if(!curl){curl_global_cleanup();output.Pause();return "Falha ao criar sessao HTTPS.";}
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
    SET(CURLOPT_USERAGENT,"INFINITY360-XSDK/0.1.8.2");
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
    if(t.cancelled)return output.Pause()?"Parcial salvo. START retoma.":output.Error();
    if(!options){output.Pause();return "Configuracao HTTPS indisponivel.";}
    if(t.invalidResponse){output.Pause();return "Resposta HTTP/Range invalida. RB duas vezes descarta parcial.";} 
    if(result!=CURLE_OK){
        if(!output.Error().empty())return output.Error();
        const std::string error=std::string("HTTPS: ")+curl_easy_strerror(result);
        return output.Pause()?error+" | Parcial salvo.":error;
    }
    if(!t.headersReady||code!=(t.offset?206:200)){output.Pause();return "Resposta HTTP invalida.";}
    if(!progress(output.Done(),item.bytes))return output.Pause()?"Parcial salvo. START retoma.":output.Error();
    success=output.Finish();
    return success?(output.Error().empty()?"Pacote verificado por SHA-256 e salvo.":output.Error()):output.Error();
}

namespace {
struct CatalogTransfer {
    std::string data;
    unsigned long long limit;
    const std::function<bool(unsigned long long,unsigned long long)>& progress;
};
size_t catalogBody(char* bytes,size_t size,size_t count,void* opaque){
    auto& t=*static_cast<CatalogTransfer*>(opaque);
    if(size&&count>std::numeric_limits<size_t>::max()/size)return 0;
    const size_t n=size*count;
    if(n>32768||n>t.limit-t.data.size())return 0;
    t.data.append(bytes,n);return n;
}
int catalogProgress(void* opaque,curl_off_t,curl_off_t,curl_off_t,curl_off_t){
    auto& t=*static_cast<CatalogTransfer*>(opaque);
    return t.progress(t.data.size(),t.limit)?0:1;
}
}
static std::string fetchMetadata(const std::string& url,unsigned long long limit,bool exact,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& data,bool& success){
    success=false;data.clear();
    std::ifstream opt((root+"enable-https-store.txt").c_str());std::string marker;
    if(!std::getline(opt,marker)||marker!="ENABLE-HTTPS-STORE")return "HTTPS bloqueado.";
    StoreItem source{};source.url=url;
    if(!limit||limit>4*1024*1024||!StoreNativeSource(source,root))return "Host ou limite recusado.";
    const std::string ca="game:/cacert.pem";
    {std::ifstream file(ca.c_str(),std::ios::binary|std::ios::ate);
     if(!file||file.tellg()<1024||file.tellg()>1024*1024)return "CA bundle ausente ou invalido.";}
    if(!progress(0,limit))return "Sincronizacao cancelada.";
    if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)return "Falha ao inicializar HTTPS.";
    CURL* curl=curl_easy_init();
    if(!curl){curl_global_cleanup();return "Falha ao criar sessao HTTPS.";}
    CatalogTransfer transfer{"",limit,progress};bool options=true;
    #define SET(k,v) do { if(curl_easy_setopt(curl,k,v)!=CURLE_OK)options=false; } while(0)
    SET(CURLOPT_URL,url.c_str());
    SET(CURLOPT_PROTOCOLS_STR,"https");
    SET(CURLOPT_FOLLOWLOCATION,0L);
    SET(CURLOPT_SSL_VERIFYPEER,1L);
    SET(CURLOPT_SSL_VERIFYHOST,2L);
    SET(CURLOPT_SSLVERSION,long(CURL_SSLVERSION_TLSv1_2|CURL_SSLVERSION_MAX_TLSv1_2));
    SET(CURLOPT_CAINFO,ca.c_str());
    SET(CURLOPT_PROXY,"");
    SET(CURLOPT_NETRC,long(CURL_NETRC_IGNORED));
    SET(CURLOPT_CONNECTTIMEOUT,15L);
    SET(CURLOPT_TIMEOUT,90L);
    SET(CURLOPT_LOW_SPEED_LIMIT,128L);
    SET(CURLOPT_LOW_SPEED_TIME,20L);
    SET(CURLOPT_FAILONERROR,1L);
    SET(CURLOPT_MAXFILESIZE_LARGE,static_cast<curl_off_t>(limit));
    SET(CURLOPT_HTTP_VERSION,long(CURL_HTTP_VERSION_1_1));
    SET(CURLOPT_USERAGENT,"INFINITY360-XSDK/0.1.9.0");
    SET(CURLOPT_WRITEFUNCTION,catalogBody);
    SET(CURLOPT_WRITEDATA,&transfer);
    SET(CURLOPT_XFERINFOFUNCTION,catalogProgress);
    SET(CURLOPT_XFERINFODATA,&transfer);
    SET(CURLOPT_NOPROGRESS,0L);
    #undef SET
    const CURLcode result=options?curl_easy_perform(curl):CURLE_FAILED_INIT;
    long status=0;curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&status);
    curl_easy_cleanup(curl);curl_global_cleanup();
    if(!options)return "Configuracao HTTPS indisponivel.";
    if(result!=CURLE_OK||status!=200||(exact&&transfer.data.size()!=limit))return "Resposta HTTPS incompleta ou recusada.";
    if(!progress(transfer.data.size(),limit))return "Consulta cancelada.";
    data.swap(transfer.data);success=true;return "";
}
std::string XsdkSyncProvider(const StoreProvider& provider,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,bool& success){
    success=false;FileWorkspace ws(root);
    if(!ws.CanWrite(FileWorkspace::Join(root,"UserFiles")))return "Habilite escrita na area pessoal.";
    std::string data;bool fetched=false;
    auto error=fetchMetadata(provider.url,provider.bytes,true,root,progress,data,fetched);
    if(!fetched)return error+" | Cache anterior preservado.";
    StoreCatalog catalog(root);success=catalog.ImportProvider(provider,data,error);
    return success?"Catalogo verificado e salvo para uso offline.":error.empty()?"Catalogo recusado.":error;
}
std::string XsdkUnityUpdates(const XboxExecutionInfo& info,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::vector<UnityUpdate>& updates,bool& success){
    success=false;updates.clear();
    std::ifstream opt((root+"enable-xboxunity.txt").c_str());std::string marker;
    if(!std::getline(opt,marker)||marker!="ENABLE-XBOXUNITY")return "Habilite enable-xboxunity.txt.";
    const std::string url=UnityCatalog::UpdatesUrl(info);
    if(url.empty())return "Title ID ou Base Version desconhecidos.";
    std::string data;bool fetched=false;
    const auto error=fetchMetadata(url,256*1024,false,root,progress,data,fetched);
    if(!fetched)return error;
    success=UnityCatalog::ParseUpdates(data,updates);
    return success?"Lista XboxUnity recebida; compatibilidade ainda nao confirmada.":"Resposta XboxUnity invalida.";
}

namespace {
struct TuTransfer {
    const std::function<bool(const unsigned char*,size_t)>& write;
    const std::function<bool(unsigned long long,unsigned long long)>& progress;
    unsigned long long bytes,done=0;
};
size_t tuBody(char* bytes,size_t size,size_t count,void* opaque){
    auto& t=*static_cast<TuTransfer*>(opaque);
    if(size&&count>std::numeric_limits<size_t>::max()/size)return 0;
    const size_t n=size*count;
    if(n>32768||n>t.bytes-t.done||!t.write(reinterpret_cast<unsigned char*>(bytes),n))return 0;
    t.done+=n;return n;
}
int tuStatus(void* opaque,curl_off_t,curl_off_t,curl_off_t,curl_off_t){
    auto& t=*static_cast<TuTransfer*>(opaque);return t.progress(t.done,t.bytes)?0:1;
}
}
std::string XsdkUnityDownload(const XboxExecutionInfo& info,const UnityUpdate& selected,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,bool& success){
    success=false;
    std::vector<UnityUpdate> current;bool listed=false;
    auto message=XsdkUnityUpdates(info,root,progress,current,listed);
    if(!listed)return message;
    UnityUpdate item{};
    for(const auto& candidate:current)if(candidate.id==selected.id)item=candidate;
    if(!item.id||item.url.empty()||item.url!=selected.url||!item.bytes||item.bytes!=selected.bytes)return "Catalogo mudou; consulte e confirme novamente.";
    StoreItem source{};source.url=item.url;
    if(!StoreNativeSource(source,root))return "Host de TU nao autorizado.";
    std::string response,md5;bool fetched=false;
    message=fetchMetadata(UnityCatalog::Md5Url(item.id),4096,false,root,progress,response,fetched);
    if(!fetched||!UnityCatalog::ParseMd5(response,md5))return "MD5 XboxUnity indisponivel.";
    FileWorkspace ws(root);std::string error;
    success=UnityStageUpdate(ws,info,item,md5,root,[&](const std::function<bool(const unsigned char*,size_t)>& write){
        if(curl_global_init(CURL_GLOBAL_DEFAULT)!=CURLE_OK)return false;
        CURL* curl=curl_easy_init();
        if(!curl){curl_global_cleanup();return false;}
        bool options=true;TuTransfer t{write,progress,item.bytes};
        #define SET(k,v) do { if(curl_easy_setopt(curl,k,v)!=CURLE_OK)options=false; } while(0)
        SET(CURLOPT_URL,item.url.c_str());SET(CURLOPT_PROTOCOLS_STR,"https");
        SET(CURLOPT_FOLLOWLOCATION,0L);SET(CURLOPT_SSL_VERIFYPEER,1L);SET(CURLOPT_SSL_VERIFYHOST,2L);
        SET(CURLOPT_SSLVERSION,long(CURL_SSLVERSION_TLSv1_2|CURL_SSLVERSION_MAX_TLSv1_2));
        SET(CURLOPT_CAINFO,"game:/cacert.pem");SET(CURLOPT_PROXY,"");SET(CURLOPT_NETRC,long(CURL_NETRC_IGNORED));
        SET(CURLOPT_CONNECTTIMEOUT,15L);SET(CURLOPT_TIMEOUT,600L);SET(CURLOPT_LOW_SPEED_LIMIT,1024L);SET(CURLOPT_LOW_SPEED_TIME,30L);
        SET(CURLOPT_FAILONERROR,1L);SET(CURLOPT_MAXFILESIZE_LARGE,static_cast<curl_off_t>(item.bytes));
        SET(CURLOPT_HTTP_VERSION,long(CURL_HTTP_VERSION_1_1));SET(CURLOPT_USERAGENT,"INFINITY360-XSDK/0.1.9.1");
        SET(CURLOPT_WRITEFUNCTION,tuBody);SET(CURLOPT_WRITEDATA,&t);SET(CURLOPT_XFERINFOFUNCTION,tuStatus);SET(CURLOPT_XFERINFODATA,&t);SET(CURLOPT_NOPROGRESS,0L);
        #undef SET
        const CURLcode result=options?curl_easy_perform(curl):CURLE_FAILED_INIT;
        long code=0;curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&code);
        curl_easy_cleanup(curl);curl_global_cleanup();
        return result==CURLE_OK&&code==200&&t.done==item.bytes;
    },progress,error);
    return success?"TU verificada em UserFiles/TitleUpdates; nao ativada.":error;
}

std::string XsdkUnityCover(unsigned title,const std::string& root,const std::function<bool(unsigned long long,unsigned long long)>& progress,bool& success){
    success=false;std::ifstream opt((root+"enable-xboxunity.txt").c_str());std::string marker;
    if(!std::getline(opt,marker)||marker!="ENABLE-XBOXUNITY")return "Habilite enable-xboxunity.txt.";
    if(!title)return "Title ID desconhecido.";
    FileWorkspace ws(root);const auto personal=FileWorkspace::Join(root,"UserFiles");
    if(!ws.CanWrite(personal))return "Habilite escrita na area pessoal.";
    std::string data;bool fetched=false;
    auto error=fetchMetadata(UnityCatalog::CoversUrl(title),256*1024,false,root,progress,data,fetched);
    if(!fetched)return error;
    std::vector<unsigned> images;
    if(!UnityCatalog::ParseCovers(data,title,images)||images.empty())return "Nenhuma capa valida para este Title ID.";
    error=fetchMetadata(UnityCatalog::CoverUrl(images.front()),4*1024*1024,false,root,progress,data,fetched);
    if(!fetched)return error;
    success=ArtImport::Save(ws,root,title,true,data,error);
    return success?"Capa XboxUnity salva na area pessoal.":error;
}
