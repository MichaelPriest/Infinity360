#include "CollectionStore.h"
#include "SavedFile.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>

namespace {
const size_t MaxCollections=16;
const size_t MaxGamesInCollection=512;
const size_t MaxDataBytes=256*1024;
const size_t InvalidIndex=static_cast<size_t>(-1);

bool validPath(const std::string& path) {
    if(path.empty() || path.size()>2048) return false;
    for(size_t i=0;i<path.size();++i) {
        const unsigned char ch=(unsigned char)path[i];
        if(ch<32u || ch==127u) return false;
    }
    return true;
}
std::string folded(std::string value) {
    for(size_t i=0;i<value.size();++i)
        value[i]=(char)std::tolower((unsigned char)value[i]);
    return value;
}
}

CollectionStore::CollectionStore(const std::string& file):file_(file) {}

bool CollectionStore::ValidName(const std::string& name) {
    if(name.empty() || name.size()>32 ||
       name[0]==' ' || name[name.size()-1]==' ') return false;
    for(size_t i=0;i<name.size();++i) {
        const unsigned char ch=(unsigned char)name[i];
        if(!(std::isalnum(ch) || ch==' ' || ch=='-' || ch=='_')) return false;
    }
    return true;
}

bool CollectionStore::Unique(const std::string& name,size_t skip) const {
    const std::string foldedName=folded(name);
    for(size_t i=0;i<collections_.size();++i) {
        if(i!=skip && folded(collections_[i].name)==foldedName)
            return false;
    }
    return true;
}

bool CollectionStore::Create(const std::string& name) {
    if(collections_.size()>=MaxCollections || !ValidName(name) ||
       !Unique(name,InvalidIndex)) return false;
    GameCollection collection;
    collection.name=name;
    collections_.push_back(collection);
    return true;
}

bool CollectionStore::Rename(size_t index,const std::string& name) {
    if(index>=collections_.size() || !ValidName(name) ||
       !Unique(name,index)) return false;
    collections_[index].name=name;
    return true;
}

bool CollectionStore::Remove(size_t index) {
    if(index>=collections_.size()) return false;
    collections_.erase(collections_.begin()+index);
    return true;
}

bool CollectionStore::Contains(size_t index,const std::string& path) const {
    if(index>=collections_.size()) return false;
    const std::vector<std::string>& entries=collections_[index].games;
    return std::find(entries.begin(),entries.end(),path)!=entries.end();
}

bool CollectionStore::Toggle(size_t index,const std::string& path) {
    if(index>=collections_.size() || !validPath(path)) return false;
    std::vector<std::string>& games=collections_[index].games;
    const std::vector<std::string>::iterator item=
        std::find(games.begin(),games.end(),path);
    if(item!=games.end()) { games.erase(item); return true; }
    if(games.size()>=MaxGamesInCollection) return false;
    games.push_back(path);
    return true;
}

bool CollectionStore::Load() {
    collections_.clear();
    std::ifstream input(savedFile(file_).c_str(),std::ios::binary);
    if(!input) return true;  // First install; empty catalogue.
    std::string line;
    size_t bytes=0;
    size_t current=InvalidIndex;
    bool valid=true;
    while(std::getline(input,line)) {
        bytes+=line.size()+1;
        if(bytes>MaxDataBytes || line.size()>2100) {valid=false;break;}
        if(!line.empty() && line[line.size()-1]=='\r')line.erase(line.size()-1);
        if(line.empty() || line[0]=='#') continue;
        if(line.size()>2 && line[0]=='C' && line[1]=='\t') {
            const std::string name=line.substr(2);
            if(!Create(name)) {current=InvalidIndex;valid=false;}
            else current=collections_.size()-1;
        } else if(line.size()>2 && line[0]=='G' && line[1]=='\t') {
            const std::string path=line.substr(2);
            if(current==InvalidIndex || !validPath(path) ||
               (collections_[current].games.size()>=MaxGamesInCollection &&
                !Contains(current,path))) {valid=false;continue;}
            if(!Contains(current,path)) collections_[current].games.push_back(path);
        } else valid=false;
    }
    return valid && !input.bad();
}

bool CollectionStore::Save() const {
    const std::string temporary=file_+".tmp";
    std::ofstream out(temporary.c_str(),std::ios::binary|std::ios::trunc);
    if(!out) return false;
    out<<"# AuroraX collections v1\n";
    for(size_t i=0;i<collections_.size();++i) {
        out<<"C\t"<<collections_[i].name<<"\n";
        const std::vector<std::string>& games=collections_[i].games;
        for(size_t j=0;j<games.size();++j)
            out<<"G\t"<<games[j]<<"\n";
    }
    out.close();
    return out.good() && replaceSavedFile(file_);
}
