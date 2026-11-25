#pragma once
#include <cstdint>
#include <string>
#include <map>
#include <ctime>
#include <ostream>

class LaunchStatistics {
public:
    struct Entry {unsigned attempts=0,failed=0;uint64_t last=0;};
    static constexpr unsigned MaxCount=1000000;
    static constexpr size_t MaxEntries=4096;
    static bool ValidPath(const std::string& path){
        if(path.empty()||path.size()>1800)return false;
        for(unsigned char c:path)if(c<32||c==127)return false;
        return true;
    }
    static bool ParseNumber(const std::string& s,uint64_t max,uint64_t& out){
        if(s.empty()||s.size()>12)return false;
        uint64_t n=0;for(char c:s){if(c<'0'||c>'9')return false;const auto d=uint64_t(c-'0');if(d>max||n>(max-d)/10)return false;n=n*10+d;}
        out=n;return true;
    }
    void Clear(){entries_.clear();pending_.clear();}
    bool LoadLine(const std::string& line){
        const auto a=line.find('\t'),b=a==std::string::npos?a:line.find('\t',a+1),c=b==std::string::npos?b:line.find('\t',b+1);
        if(c==std::string::npos)return false;
        const auto path=line.substr(c+1);uint64_t attempts=0,failed=0,last=0;
        if(!ValidPath(path)||!ParseNumber(line.substr(0,a),MaxCount,attempts)||!attempts||
           !ParseNumber(line.substr(a+1,b-a-1),attempts,failed)||
           !ParseNumber(line.substr(b+1,c-b-1),4102444800ULL,last)||
           entries_.count(path)||entries_.size()>=MaxEntries)return false;
        entries_[path]={static_cast<unsigned>(attempts),static_cast<unsigned>(failed),last};return true;
    }
    void Write(std::ostream& out)const{
        for(const auto& kv:entries_)out<<"S\t"<<kv.second.attempts<<"\t"<<kv.second.failed<<"\t"<<kv.second.last<<"\t"<<kv.first<<"\n";
    }
    bool Attempt(const std::string& path,uint64_t now){
        pending_.clear();
        if(!ValidPath(path)||(!entries_.count(path)&&entries_.size()>=MaxEntries))return false;
        auto& e=entries_[path];if(e.attempts>=MaxCount)return false;
        ++e.attempts;e.last=now>=946684800ULL&&now<=4102444800ULL?now:0;
        pending_=path;return true;
    }
    bool Failed(const std::string& path){
        if(pending_.empty()||pending_!=path)return false;
        pending_.clear();auto it=entries_.find(path);
        if(it==entries_.end()||it->second.failed>=it->second.attempts)return false;
        ++it->second.failed;return true;
    }
    Entry Get(const std::string& path)const{auto i=entries_.find(path);return i==entries_.end()?Entry{}:i->second;}
    static std::string Date(uint64_t last){
        if(!last)return "indisponivel";
        const auto t=static_cast<std::time_t>(last);
        if(t<0||static_cast<uint64_t>(t)!=last)return "indisponivel";
        char text[32]{};const auto* local=std::localtime(&t);
        return local&&std::strftime(text,sizeof text,"%d/%m/%Y %H:%M",local)?text:"indisponivel";
    }
private:
    std::map<std::string,Entry> entries_;std::string pending_;
};
