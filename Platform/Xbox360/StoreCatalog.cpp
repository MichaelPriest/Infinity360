#include "StoreCatalog.h"
#include "StorePartial.h"
#include "StoreInstallPath.h"
#include "XexLaunchCheck.h"
#include "SavedFile.h"
#include "FileWorkspace.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <set>
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
}
bool StoreCatalog::Load() {
    items_.clear();requests_.clear();validRequests_=true;
    native_.clear();validNative_=true;
    const std::string nativePath=savedFile(root_+"infinity360-native-queue.tsv");
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
    if(!readBound(root_+"aurorax-store.tsv",1024*1024,data))return false;
    std::istringstream in(data);std::set<std::string> ids;
    while(std::getline(in,line)) {
        if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
        if(line.empty()||line[0]=='#')continue;
        StoreItem item;
        if(items_.size()>=1024||line.size()>1400||!parse(line,item)||!ids.insert(item.id).second) {
            items_.clear();return false;
        }
        items_.push_back(item);
    }
    return true;
}
bool StoreCatalog::Requested(const StoreItem& item) const {
    return std::find(requests_.begin(),requests_.end(),key(item))!=requests_.end();
}
bool StoreCatalog::SaveRequests() {
    if(!validRequests_)return false;
    const std::string file=root_+"aurorax-store-requests.tsv";
    std::ofstream out((file+".tmp").c_str(),std::ios::binary|std::ios::trunc);
    for(size_t i=0;i<requests_.size();++i)out<<requests_[i]<<"\n";
    out.close();return out&&replaceSavedFile(file);
}
bool StoreCatalog::Request(const StoreItem& item) {
    if(!validRequests_)return false;
    if(Requested(item))return true;
    if(requests_.size()>=64)return false;
    requests_.push_back(key(item));
    if(SaveRequests())return true;
    requests_.pop_back();return false;
}
bool StoreCatalog::Cancel(const StoreItem& item) {
    const std::vector<std::string> old=requests_;
    requests_.erase(std::remove(requests_.begin(),requests_.end(),key(item)),requests_.end());
    if(SaveRequests())return true;
    requests_=old;return false;
}
bool StoreCatalog::NativeQueued(const StoreItem& item) const {
    return validNative_&&std::find(native_.begin(),native_.end(),key(item))!=native_.end();
}
bool StoreCatalog::SaveNative(){
    if(!validNative_)return false;
    const std::string path=root_+"infinity360-native-queue.tsv";
    std::ofstream out((path+".tmp").c_str(),std::ios::binary|std::ios::trunc);
    for(const auto& entry:native_)out<<entry<<"\n";
    out.close();return out&&replaceSavedFile(path);
}
bool StoreCatalog::QueueNative(const StoreItem& item){
    if(!validNative_||!token(item.id,32)||!hash(item.sha256)||!item.bytes||item.bytes>512ULL*1024*1024)return false;
    if(NativeQueued(item))return true;
    if(native_.size()>=64)return false;
    native_.push_back(key(item));
    if(SaveNative())return true;
    native_.pop_back();return false;
}
bool StoreCatalog::RemoveNative(const StoreItem& item){
    if(!validNative_)return false;
    if(!NativeQueued(item))return true;
    const auto old=native_;
    native_.erase(std::remove(native_.begin(),native_.end(),key(item)),native_.end());
    if(SaveNative())return true;
    native_=old;return false;
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
bool StoreCatalog::ListedDownload(const StoreItem& item) const {
    const StoreProgress::State state=Progress(item).state;
    return Requested(item)||NativeQueued(item)||state==StoreProgress::NativeVerified||state==StoreProgress::NativePaused;
}
bool StoreCatalog::Installed(const StoreItem& item) const{
    const std::string folder=StoreInstallPath::Folder(root_,item);
    if(folder.empty())return false;
    std::ifstream input(FileWorkspace::Join(folder,"default.xex").c_str(),std::ios::binary|std::ios::ate);
    return input&&input.tellg()>=0&&static_cast<unsigned long long>(input.tellg())==item.bytes&&XexLaunchCheck::Header(input);
}
bool StoreCatalog::Staged(const StoreItem& item) const {
    // Size only. The PC worker checks SHA-256, and no package is auto-executed.
    const std::string path=FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(root_,"UserFiles"),"Store"),item.id+".pkg");
    std::ifstream in(path.c_str(),std::ios::binary|std::ios::ate);
    return in&&in.tellg()>=0&&static_cast<unsigned long long>(in.tellg())==item.bytes;
}

bool StoreCatalog::DiscardPartial(const StoreItem& item) {
    const std::string folder=FileWorkspace::Join(FileWorkspace::Join(root_,"UserFiles"),"Store");
    const std::string path=StorePartial::Path(folder,item);
    FileWorkspace workspace(root_);
    return !path.empty()&&workspace.CanReadPersonalFile(path)&&std::remove(path.c_str())==0;
}
StoreProgress StoreCatalog::Progress(const StoreItem& item) const {
    StoreProgress result;
    const std::string folder=FileWorkspace::Join(FileWorkspace::Join(root_,"UserFiles"),"Store");
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
