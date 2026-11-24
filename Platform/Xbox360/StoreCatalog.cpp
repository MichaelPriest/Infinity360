#include "StoreCatalog.h"
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
    return s=="game"||s=="xbla"||s=="homebrew"||s=="app"||s=="emulator"||s=="dlc";
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
    if(fields.size()!=8||!token(fields[0],32)||!category(fields[1])||
       !display(fields[2],96)||!display(fields[3],24)||!hash(fields[5])||
       fields[6].compare(0,8,"https://")!=0||!display(fields[6],1024)||
       !display(fields[7],96))return false;
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
bool StoreCatalog::Staged(const StoreItem& item) const {
    // Size only. The PC worker checks SHA-256, and no package is auto-executed.
    const std::string path=FileWorkspace::Join(FileWorkspace::Join(FileWorkspace::Join(root_,"UserFiles"),"Store"),item.id+".pkg");
    std::ifstream in(path.c_str(),std::ios::binary|std::ios::ate);
    return in&&in.tellg()>=0&&static_cast<unsigned long long>(in.tellg())==item.bytes;
}
