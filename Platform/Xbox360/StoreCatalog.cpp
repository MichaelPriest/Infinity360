#include "StoreCatalog.h"
#include "StoreStorage.h"
#include "StorePartial.h"
#include "StoreSha256.h"
#include "MediaUrl.h"
#include "StoreInstallPath.h"
#include "XexLaunchCheck.h"
#include "SavedFile.h"
#include "FileWorkspace.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <set>
#include <cstdio>
namespace {
bool token(const std::string& s,size_t max) {
    if(s.empty()||s.size()>max)return false;
    for(size_t i=0;i<s.size();++i) {
        char ch=s[i];
        if(!((ch>='a'&&ch<='z')||(ch>='0'&&ch<='9')||ch=='-'||ch=='_'))return false;
    }
    return true;
}
bool display(const std::string& s,size_t max) {
    if(s.empty()||s.size()>max)return false;
    for(size_t i=0;i<s.size();++i)if((unsigned char)s[i]<32||s[i]==127)return false;
    return true;
}
bool hash(const std::string& s) {
    if(s.size()!=64)return false;
    for(size_t i=0;i<s.size();++i)
        if(!((s[i]>='0'&&s[i]<='9')||(s[i]>='a'&&s[i]<='f')))return false;
    return true;
}
bool category(const std::string& s) {
    return s=="game"||s=="xbla"||s=="homebrew"||s=="app"||s=="emulator"||s=="dlc"||s=="translation";
}
bool hex8(const std::string& s,unsigned& value) {
    if(s.size()!=8)return false;
    value=0;
    for(size_t i=0;i<s.size();++i) {
        const char ch=s[i];
        if(ch>='0'&&ch<='9')value=(value<<4)|(ch-'0');
        else if(ch>='A'&&ch<='F')value=(value<<4)|(ch-'A'+10);
        else return false;
    }
    return value!=0;
}
bool readBound(const std::string& path,size_t cap,std::string& data) {
    std::ifstream in(path.c_str(),std::ios::binary);
    if(!in)return false;
    std::vector<char> buf(cap+1);
    in.read(&buf[0],static_cast<std::streamsize>(buf.size()));
    if(in.bad()||in.gcount()>static_cast<std::streamsize>(cap))return false;
    data.assign(&buf[0],static_cast<size_t>(in.gcount()));return true;
}
bool parse(const std::string& line,StoreItem& item) {
    std::vector<std::string> fields;
    size_t at=0;
    for(size_t i=0;i<=line.size();++i)if(i==line.size()||line[i]=='\t') {
        fields.push_back(line.substr(at,i-at));at=i+1;
    }
    if((fields.size()!=8&&fields.size()!=11)||!token(fields[0],32)||!category(fields[1])||
       !display(fields[2],96)||!display(fields[3],24)||!hash(fields[5])||
       fields[6].compare(0,8,"https://")!=0||!display(fields[6],1024)||
       !display(fields[7],96))return false;
    item.titleId=item.mediaId=item.gameVersion=0;
    if(fields[1]=="translation") {
        if(fields.size()!=11||!hex8(fields[8],item.titleId)||
           !hex8(fields[9],item.mediaId)||!hex8(fields[10],item.gameVersion))return false;
    } else if(fields.size()!=8)return false;
    unsigned long long size=0;
    if(fields[4].empty()||fields[4].size()>12)return false;
    for(size_t i=0;i<fields[4].size();++i) {
        char ch=fields[4][i];if(ch<'0'||ch>'9')return false;
        size=size*10+(ch-'0');
    }
    if(!size||size>8589934592ULL)return false;
    item.id=fields[0];item.category=fields[1];item.name=fields[2];item.version=fields[3];
    item.bytes=size;item.sha256=fields[5];item.url=fields[6];item.rights=fields[7];return true;
}
std::string key(const StoreItem& item){return item.id+"\t"+item.sha256;}
constexpr size_t MaxProviderBytes=256*1024;
bool secureUrl(const std::string& value){
    MediaUrl parsed;
    return value.compare(0,8,"https://")==0&&value.size()<=1024&&value.find('#')==std::string::npos&&
        parsed.Parse(value)&&parsed.secure&&parsed.port==443;
}
bool readProviders(const std::string& root,std::vector<StoreProvider>& providers){
    providers.clear();const std::string path=root+"infinity360-providers.tsv";
    if(!fileExists(path))return true;
    std::string data,line;if(!readBound(path,16384,data))return false;
    std::set<std::string> ids;std::istringstream rows(data);
    while(std::getline(rows,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty()||line[0]=='#')continue;
        std::vector<std::string> fields;size_t at=0;
        for(size_t i=0;i<=line.size();++i)if(i==line.size()||line[i]=='\t'){fields.push_back(line.substr(at,i-at));at=i+1;}
        if(fields.size()!=5||providers.size()>=8||!token(fields[0],12)||!display(fields[1],64)||
           !secureUrl(fields[2])||!hash(fields[4])||!ids.insert(fields[0]).second||
           fields[3].empty()||fields[3].size()>6)return false;
        unsigned long long bytes=0;
        for(char ch:fields[3]){if(ch<'0'||ch>'9')return false;bytes=bytes*10+ch-'0';}
        if(!bytes||bytes>MaxProviderBytes)return false;
        StoreProvider provider;provider.id=fields[0];provider.name=fields[1];provider.url=fields[2];provider.bytes=bytes;provider.sha256=fields[4];
        providers.push_back(provider);
    }
    return true;
}
std::string providerFolder(const std::string& root){
    return FileWorkspace::Join(FileWorkspace::Join(root,"UserFiles"),"StoreCatalogs");
}
std::string providerFile(const std::string& root,const StoreProvider& provider){
    return FileWorkspace::Join(providerFolder(root),provider.id+".tsv");
}
bool providerItems(const StoreProvider& provider,const std::string& data,const std::vector<StoreItem>& existing,std::vector<StoreItem>& out){
    out.clear();if(data.size()!=provider.bytes||data.size()>MaxProviderBytes||data.find('\0')!=std::string::npos)return false;
    StoreSha256 sha;sha.Update(reinterpret_cast<const unsigned char*>(data.data()),data.size());
    if(sha.Finish()!=provider.sha256)return false;
    std::set<std::string> ids;size_t retained=0;
    for(const auto& item:existing)if(item.provider!=provider.id){ids.insert(item.id);++retained;}
    std::istringstream rows(data);std::string line;
    while(std::getline(rows,line)){
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.empty()||line[0]=='#')continue;
        StoreItem item;
        if(out.size()>=256||retained+out.size()>=1024||line.size()>1400||!parse(line,item)||
           !secureUrl(item.url)||item.bytes>512ULL*1024*1024)return false;
        item.id=provider.id+"-"+item.id;item.provider=provider.id;
        if(!token(item.id,32)||!ids.insert(item.id).second)return false;
        out.push_back(item);
    }
    return true;
}
#include "Modules/Store/AndromedaImport.inc"
}
bool StoreCatalog::Load() {
    items_.clear();requests_.clear();validRequests_=true;
    providers_.clear();providersValid_=true;
    storage_=StoreStorage::Read(root_);
    native_.clear();validNative_=StoreStorage::Valid(storage_)&&!StoreStorage::Personal(root_,storage_).empty();
    const std::string nativePath=validNative_?savedFile(StoreStorage::Queue(root_,storage_)):"";
    if(fileExists(nativePath)){
        std::string data,line;
        if(!readBound(nativePath,8192,data))validNative_=false;
        else{
            std::istringstream input(data);
            while(std::getline(input,line)){
                if(!line.empty()&&line.back()=='\r')line.pop_back();
                const size_t tab=line.find('\t');
                if(tab==std::string::npos||!token(line.substr(0,tab),32)||!hash(line.substr(tab+1))||
                   native_.size()>=64||std::find(native_.begin(),native_.end(),line)!=native_.end()){
                    validNative_=false;break;
                }
                native_.push_back(line);
            }
        }
        if(!validNative_)native_.clear();
    }
    const std::string queue=root_+"aurorax-store-requests.tsv";
    const std::string qpath=savedFile(queue);
    std::string data,line;
    if(fileExists(qpath)) {
        if(!readBound(qpath,8192,data))validRequests_=false;
        else {
            std::istringstream in(data);
            while(std::getline(in,line)) {
                if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
                const size_t tab=line.find('\t');
                if(tab==std::string::npos||!token(line.substr(0,tab),32)||!hash(line.substr(tab+1))||
                   requests_.size()>=64) {validRequests_=false;break;}
                if(std::find(requests_.begin(),requests_.end(),line)==requests_.end())requests_.push_back(line);
            }
        }
    }
    const bool localPresent=fileExists(root_+"aurorax-store.tsv");
    if(localPresent){
        if(!readBound(root_+"aurorax-store.tsv",1024*1024,data))return false;
        std::istringstream in(data);std::set<std::string> ids;
        while(std::getline(in,line)){
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line[0]=='#')continue;
            StoreItem item;
            if(items_.size()>=1024||line.size()>1400||!parse(line,item)||!ids.insert(item.id).second){
                items_.clear();return false;
            }
            item.storage=storage_;items_.push_back(item);
        }
    }
    providersValid_=readProviders(root_,providers_);
    if(!providersValid_){providers_.clear();AndromedaImport::Load(root_,storage_,items_);return localPresent||!items_.empty();}
    FileWorkspace ws(root_);
    for(auto& provider:providers_){
        const auto path=savedFile(providerFile(root_,provider));
        std::vector<StoreItem> entries;
        if(ws.CanReadPersonalFile(path)&&readBound(path,MaxProviderBytes,data)&&providerItems(provider,data,items_,entries)){
            provider.cached=true;provider.items=entries.size();
            for(auto& item:entries)item.storage=storage_;
            items_.insert(items_.end(),entries.begin(),entries.end());
        }
    }
    AndromedaImport::Load(root_,storage_,items_);
    return localPresent||!providers_.empty()||!items_.empty();
}
bool StoreCatalog::SelectStorage(const std::string& id){
    FileWorkspace ws(root_);if(!StoreStorage::Save(ws,root_,id))return false;
    Load();return true;
}
bool StoreCatalog::ImportProvider(const StoreProvider& provider,const std::string& data,std::string& error){
    error.clear();
    // Re-read local authorization: a stale UI request cannot import a revoked pin.
    StoreCatalog current(root_);current.Load();
    if(!current.ProvidersValid()){error="Configuracao de provedores invalida.";return false;}
    const auto it=std::find_if(current.providers_.begin(),current.providers_.end(),[&](const StoreProvider& p){
        return p.id==provider.id&&p.url==provider.url&&p.sha256==provider.sha256&&p.bytes==provider.bytes;
    });
    if(it==current.providers_.end()){error="Provedor removido ou pin alterado.";return false;}
    std::vector<StoreItem> entries;
    if(!providerItems(*it,data,current.items_,entries)){error="Catalogo recusado: hash, tamanho, formato ou IDs.";return false;}
    FileWorkspace ws(root_);
    const auto personal=FileWorkspace::Join(root_,"UserFiles");
    const auto folder=providerFolder(root_);
    if(!ws.CanWrite(personal)||(!ws.CanWrite(folder)&&!ws.Create(personal,"StoreCatalogs"))){
        error="Area pessoal indisponivel; habilite escrita.";return false;
    }
    const auto path=providerFile(root_,*it);
    // No replacement of local catalog or packages; only this provider's cache.
    std::ofstream output((path+".tmp").c_str(),std::ios::binary|std::ios::trunc);
    output.write(data.data(),static_cast<std::streamsize>(data.size()));output.close();
    if(!output||!replaceSavedFile(path)){error="Falha ao salvar cache.";return false;}
    return Load();
}
bool StoreCatalog::Requested(const StoreItem& item) const {
    return item.storage.empty()&&std::find(requests_.begin(),requests_.end(),key(item))!=requests_.end();
}
bool StoreCatalog::SaveRequests() {
    if(!validRequests_)return false;
    const std::string file=root_+"aurorax-store-requests.tsv";
    std::ofstream out((file+".tmp").c_str(),std::ios::binary|std::ios::trunc);
    for(size_t i=0;i<requests_.size();++i)out<<requests_[i]<<"\n";
    out.close();return out&&replaceSavedFile(file);
}
bool StoreCatalog::Request(const StoreItem& item) {
    if(!validRequests_||!item.provider.empty()||!item.storage.empty()||item.url.empty())return false;
    if(Requested(item))return true;
    if(requests_.size()>=64)return false;
    requests_.push_back(key(item));
    if(SaveRequests())return true;
    requests_.pop_back();return false;
}
bool StoreCatalog::Cancel(const StoreItem& item) {
    if(!item.storage.empty())return false;
    const std::vector<std::string> old=requests_;
    requests_.erase(std::remove(requests_.begin(),requests_.end(),key(item)),requests_.end());
    if(SaveRequests())return true;
    requests_=old;return false;
}
bool StoreCatalog::NativeQueued(const StoreItem& item) const {
    return validNative_&&item.storage==storage_&&std::find(native_.begin(),native_.end(),key(item))!=native_.end();
}
bool StoreCatalog::SaveNative(){
    if(!validNative_)return false;
    const std::string path=StoreStorage::Queue(root_,storage_);
    if(path.empty())return false;
    std::ofstream out((path+".tmp").c_str(),std::ios::binary|std::ios::trunc);
    for(const auto& entry:native_)out<<entry<<"\n";
    out.close();return out&&replaceSavedFile(path);
}
bool StoreCatalog::QueueNative(const StoreItem& item){
    if(!validNative_||item.storage!=storage_||!token(item.id,32)||!hash(item.sha256)||!item.bytes||item.bytes>512ULL*1024*1024)return false;
    if(NativeQueued(item))return true;
    if(native_.size()>=64)return false;
    native_.push_back(key(item));
    if(SaveNative())return true;
    native_.pop_back();return false;
}
bool StoreCatalog::RemoveNative(const StoreItem& item){
    if(!validNative_||item.storage!=storage_)return false;
    if(!NativeQueued(item))return true;
    const auto old=native_;
    native_.erase(std::remove(native_.begin(),native_.end(),key(item)),native_.end());
    if(SaveNative())return true;
    native_=old;return false;
}
bool StoreCatalog::PrioritizeNative(const StoreItem& item){
    if(!NativeQueued(item))return false;
    const auto found=std::find(native_.begin(),native_.end(),key(item));
    if(found==native_.begin())return true;
    const auto old=native_;
    std::rotate(native_.begin(),found,found+1);
    if(SaveNative())return true;
    native_=old;return false;
}
StoreQueueSummary StoreCatalog::QueueSummary() const{
    StoreQueueSummary result;
    if(!validNative_)return result;
    const auto queue=NativeQueue();
    result.queued=queue.size();
    result.unavailable=native_.size()-queue.size();
    for(size_t index:queue){
        const auto& item=items_[index];
        const auto progress=Progress(item);
        if(progress.state==StoreProgress::Verified||progress.state==StoreProgress::NativeVerified){
            ++result.verified;continue;
        }
        // Only native partials are resumable by the native downloader. PC state
        // files and failed attempts do not prove that reusable bytes exist.
        if(progress.state==StoreProgress::NativePaused){
            ++result.paused;
            result.remaining+=item.bytes-std::min(item.bytes,progress.received);
        }else result.remaining+=item.bytes;
    }
    return result;
}
std::vector<size_t> StoreCatalog::NativeQueue() const {
    std::vector<size_t> ordered;
    if(!validNative_)return ordered;
    for(const auto& entry:native_){
        for(size_t i=0;i<items_.size();++i)
            if(key(items_[i])==entry){ordered.push_back(i);break;}
    }
    return ordered;
}
size_t StoreCatalog::OrphanNativeCount() const{
    if(!validNative_)return 0;
    const auto count=NativeQueue().size();return native_.size()>count?native_.size()-count:0;
}
bool StoreCatalog::PruneOrphanNative(){
    if(!validNative_)return false;
    const auto old=native_;
    std::set<std::string> keys;for(const auto& item:items_)keys.insert(key(item));
    native_.erase(std::remove_if(native_.begin(),native_.end(),[&](const std::string& entry){return !keys.count(entry);}),native_.end());
    if(native_==old||SaveNative())return true;
    native_=old;return false;
}
bool StoreCatalog::ListedDownload(const StoreItem& item) const {
    const StoreProgress::State state=Progress(item).state;
    return Requested(item)||NativeQueued(item)||state==StoreProgress::NativeVerified||state==StoreProgress::NativePaused;
}
namespace {
std::string backupRoot(const std::string& root,const StoreItem& item,bool package){
    if(StoreInstallPath::Folder(root,item).empty())return "";
    return FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(
        StoreStorage::Personal(root,item.storage),"StoreBackups"),package?"Packages":StoreInstallPath::Category(item)),item.id);
}
bool enableManagement(const std::string& root){
    std::ifstream f(FileWorkspace::Join(root,"enable-store-install.txt").c_str());
    std::string line;return bool(std::getline(f,line))&&line=="ENABLE-STORE-INSTALL";
}
bool ensureBackup(const FileWorkspace& ws,const std::string& root,const StoreItem& item,bool package){
    if(!enableManagement(root)||StoreInstallPath::Folder(root,item).empty())return false;
    std::string folder=StoreStorage::Personal(root,item.storage);
    if(folder.empty())return false;
    for(const auto& name:std::vector<std::string>{"StoreBackups",package?"Packages":StoreInstallPath::Category(item),item.id}){
        const auto child=FileWorkspace::Join(folder,name);
        if(!ws.CanWrite(child)&&!ws.Create(folder,name))return false;
        folder=child;
    }
    return true;
}
std::string slot(const std::string& root,int index,bool package){
    char name[32];std::sprintf(name,package?"package-%04d.pkg":"version-%04d",index);
    return FileWorkspace::Join(root,name);
}
bool validXexFolder(const std::string& folder){
    if(folder.empty())return false;
    std::ifstream xex(FileWorkspace::Join(folder,"default.xex").c_str(),std::ios::binary);
    return XexLaunchCheck::Header(xex);
}
int newestBackup(const FileWorkspace& ws,const std::string& root,bool package){
    if(root.empty())return -1;
    for(int i=31;i>=0;--i){
        const auto path=slot(root,i,package);
        if(package?ws.CanReadPersonalFile(path):ws.CanWrite(path))return i;
    }
    return -1;
}
}
bool StoreCatalog::InstalledFolder(const StoreItem& item) const{
    const auto folder=StoreInstallPath::Folder(root_,item);
    FileWorkspace ws(root_);
    return !folder.empty()&&ws.CanBrowse(folder)&&validXexFolder(folder);
}
bool StoreCatalog::PackagePresent(const StoreItem& item) const{
    if(StoreInstallPath::Folder(root_,item).empty())return false;
    FileWorkspace ws(root_);
    return ws.CanReadPersonalFile(FileWorkspace::Join(FileWorkspace::Join(StoreStorage::Personal(root_,item.storage),"Store"),item.id+".pkg"));
}
bool StoreCatalog::BackupAvailable(const StoreItem& item) const{
    FileWorkspace ws(root_);
    const auto path=backupRoot(root_,item,false);
    const int last=newestBackup(ws,path,false);
    return last>=0&&validXexFolder(slot(path,last,false));
}
bool StoreCatalog::ArchiveInstalled(const StoreItem& item,std::string& saved){
    saved.clear();FileWorkspace ws(root_);
    const auto active=StoreInstallPath::Folder(root_,item),back=backupRoot(root_,item,false);
    if(!InstalledFolder(item)||!ensureBackup(ws,root_,item,false))return false;
    const int next=newestBackup(ws,back,false)+1;
    if(next>=32)return false;
    const auto dest=slot(back,next,false);
    if(!ws.MovePersonalExclusive(active,dest,true))return false;
    saved=dest;return true;
}
bool StoreCatalog::RestoreInstalled(const StoreItem& item,std::string& error){
    error.clear();FileWorkspace ws(root_);
    const auto active=StoreInstallPath::Folder(root_,item),back=backupRoot(root_,item,false);
    if(active.empty()||!enableManagement(root_)||!BackupAvailable(item)){error="Backup valido indisponivel.";return false;}
    const int last=newestBackup(ws,back,false);
    const auto previous=slot(back,last,false);
    std::string displaced;
    if(InstalledFolder(item)&&!ArchiveInstalled(item,displaced)){error="Nao foi possivel preservar versao ativa.";return false;}
    if(ws.MovePersonalExclusive(previous,active,true))return true;
    if(!displaced.empty()&&!ws.MovePersonalExclusive(displaced,active,true))
        error="Restauracao interrompida. Ambas versoes preservadas em StoreBackups.";
    else error="Restauracao recusada; destino ocupado ou indisponivel.";
    return false;
}
bool StoreCatalog::ArchivePackage(const StoreItem& item,std::string& saved){
    saved.clear();FileWorkspace ws(root_);
    if(!PackagePresent(item)||!ensureBackup(ws,root_,item,true))return false;
    const auto back=backupRoot(root_,item,true);
    const int next=newestBackup(ws,back,true)+1;if(next>=32)return false;
    const auto from=FileWorkspace::Join(FileWorkspace::Join(StoreStorage::Personal(root_,item.storage),"Store"),item.id+".pkg");
    const auto dest=slot(back,next,true);
    const auto state=FileWorkspace::Join(FileWorkspace::Join(StoreStorage::Personal(root_,item.storage),"Store"),item.id+".state");
    const auto receipt=FileWorkspace::Join(back,"package-"+std::string(next<10?"000":"00")+std::to_string(next)+".state");
    bool movedState=false;
    if(fileExists(state)){
        if(!ws.MovePersonalExclusive(state,receipt,false))return false;
        movedState=true;
    }
    if(!ws.MovePersonalExclusive(from,dest,false)){
        if(movedState)ws.MovePersonalExclusive(receipt,state,false);
        return false;
    }
    saved=dest;return true;
}
bool StoreCatalog::Installed(const StoreItem& item) const{
    const std::string folder=StoreInstallPath::Folder(root_,item);
    if(folder.empty())return false;
    std::ifstream input(FileWorkspace::Join(folder,"default.xex").c_str(),std::ios::binary|std::ios::ate);
    if(!input||input.tellg()<0)return false;
    const auto size=static_cast<unsigned long long>(input.tellg());
    if(!XexLaunchCheck::Header(input))return false;
    std::string record;
    const auto receipt=FileWorkspace::Join(folder,"infinity-install.txt");
    if(fileExists(receipt)&&!readBound(receipt,128,record))return false;
    if(!record.empty()||fileExists(receipt)){
        std::istringstream in(record);std::string sha,extra;unsigned long long expected=0;
        return (in>>sha>>expected)&&!(in>>extra)&&sha==item.sha256&&expected==size;
    }
    return size==item.bytes;
}
bool StoreCatalog::Staged(const StoreItem& item) const {
    if(StoreStorage::Personal(root_,item.storage).empty())return false;
    // Size only. The PC worker checks SHA-256, and no package is auto-executed.
    const std::string path=FileWorkspace::Join(FileWorkspace::Join(StoreStorage::Personal(root_,item.storage),"Store"),item.id+".pkg");
    std::ifstream in(path.c_str(),std::ios::binary|std::ios::ate);
    if(!in||in.tellg()<0||static_cast<unsigned long long>(in.tellg())!=item.bytes)return false;
    // A receipt for another version must not classify same-size bytes as current.
    std::string receipt;
    const auto state=FileWorkspace::Join(FileWorkspace::Join(StoreStorage::Personal(root_,item.storage),"Store"),item.id+".state");
    if(readBound(state,256,receipt)){
        std::istringstream record(receipt);std::string digest,status;unsigned long long size=0;
        if(record>>digest>>status>>size){
            if(hash(digest)&&(status=="native_verified"||status=="verified")&&digest!=item.sha256)return false;
        }
    }
    return true;
}

bool StoreCatalog::DiscardPartial(const StoreItem& item) {
    if(StoreStorage::Personal(root_,item.storage).empty())return false;
    const std::string folder=FileWorkspace::Join(StoreStorage::Personal(root_,item.storage),"Store");
    const std::string path=StorePartial::Path(folder,item);
    FileWorkspace workspace(root_);
    return !path.empty()&&workspace.CanReadPersonalFile(path)&&std::remove(path.c_str())==0;
}
StoreProgress StoreCatalog::Progress(const StoreItem& item) const {
    StoreProgress result;
    if(StoreStorage::Personal(root_,item.storage).empty())return result;
    const std::string folder=FileWorkspace::Join(StoreStorage::Personal(root_,item.storage),"Store");
    const std::string partial=StorePartial::Path(folder,item);
    if(!partial.empty()&&!Staged(item)){
        std::ifstream in(partial.c_str(),std::ios::binary|std::ios::ate);
        if(in&&in.tellg()>=0&&static_cast<unsigned long long>(in.tellg())<=item.bytes){
            result.state=StoreProgress::NativePaused;result.received=static_cast<unsigned long long>(in.tellg());return result;
        }
    }
    std::string data;
    if(!readBound(FileWorkspace::Join(folder,item.id+".state"),256,data))return result;
    std::istringstream in(data);std::string digest,state,bytes,extra;
    if(!(in>>digest>>state>>bytes)||in>>extra||digest!=item.sha256||bytes.empty()||bytes.size()>12)return result;
    unsigned long long size=0;
    for(size_t i=0;i<bytes.size();++i) {
        if(bytes[i]<'0'||bytes[i]>'9')return result;
        size=size*10+(bytes[i]-'0');
    }
    if(size>item.bytes)return result;
    if(state=="receiving")result.state=StoreProgress::Receiving;
    else if(state=="failed")result.state=StoreProgress::Failed;
    else if(state=="verified" && size==item.bytes && Staged(item))result.state=StoreProgress::Verified;
    else if(state=="native_verified" && size==item.bytes && Staged(item))result.state=StoreProgress::NativeVerified;
    else return result;
    result.received=size;
    return result;
}
