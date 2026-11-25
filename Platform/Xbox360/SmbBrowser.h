#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

// Portable navigation/selection model. A pending path becomes current only
// after the remote directory query succeeds; never silently reuse stale rows.
class SmbBrowser {
public:
    struct Entry {std::string name;uint64_t size=0;bool directory=false;};
    struct CopyPlan {std::string remote,name;uint64_t bytes=0;};
    static bool Name(const std::string& name){
        if(name.empty()||name.size()>255||name=="."||name=="..")return false;
        for(unsigned char c:name)if(c<32||c==127||c=='/'||c=='\\'||c==':')return false;
        return true;
    }
    static bool LocalName(const std::string& name){
        if(!Name(name)||name.size()>42||name.front()==' '||name.back()==' '||name.back()=='.')return false;
        std::string upper;
        for(unsigned char c:name){
            if(!((c>='A'&&c<='Z')||(c>='a'&&c<='z')||(c>='0'&&c<='9')||c==' '||c=='.'||c=='-'||c=='_'))return false;
            upper+=char(c>='a'&&c<='z'?c-32:c);
        }
        if(upper.compare(0,7,"AXCOPY-")==0)return false;
        const std::string stem=upper.substr(0,upper.find('.'));
        if(stem=="CON"||stem=="PRN"||stem=="AUX"||stem=="NUL"||
           (stem.size()==4&&(stem.compare(0,3,"COM")==0||stem.compare(0,3,"LPT")==0)&&stem[3]>='1'&&stem[3]<='9'))return false;
        return true;
    }
    static std::string Join(const std::string& parent,const std::string& name){
        if(!Name(name))return "";
        const auto path=parent.empty()?name:parent+"/"+name;
        return path.size()<=1024?path:"";
    }
    bool BeginRoot(){return Begin("");}
    bool BeginChild(){
        if(pending_||row_>=rows_.size()||!rows_[row_].directory)return false;
        auto next=Join(path_,rows_[row_].name);return !next.empty()&&Begin(next);
    }
    bool BeginParent(){
        if(pending_||path_.empty())return false;
        const size_t at=path_.find_last_of('/');
        return Begin(at==std::string::npos?"":path_.substr(0,at));
    }
    bool BeginRefresh(){return Begin(path_);}
    void Abort(){pending_=false;draft_.clear();}
    bool Commit(std::vector<Entry> entries,bool truncated){
        if(!pending_)return false;
        std::vector<Entry> valid;
        for(auto& e:entries){
            if(!Name(e.name))continue;
            if(valid.size()>=512){truncated=true;break;}
            valid.push_back(std::move(e));
        }
        std::sort(valid.begin(),valid.end(),[](const Entry& a,const Entry& b){
            return a.directory!=b.directory?a.directory>b.directory:a.name<b.name;
        });
        valid.erase(std::unique(valid.begin(),valid.end(),[](const Entry& a,const Entry& b){
            return a.directory==b.directory&&a.name==b.name;
        }),valid.end());
        path_=draft_;draft_.clear();pending_=false;rows_=std::move(valid);row_=0;truncated_=truncated;return true;
    }
    void Reset(){*this=SmbBrowser();}
    void Select(size_t row){if(!pending_&&row<rows_.size())row_=row;}
    bool Plan(CopyPlan& plan)const{
        plan=CopyPlan{};
        if(pending_||row_>=rows_.size()||rows_[row_].directory||!LocalName(rows_[row_].name))return false;
        const auto& e=rows_[row_];const auto path=Join(path_,e.name);
        if(path.empty()||e.size>0xffffffffULL)return false; // FATX single file limit
        plan={path,e.name,e.size};return true;
    }
    bool Pending()const{return pending_;}
    bool Truncated()const{return truncated_;}
    size_t Row()const{return row_;}
    const std::string& Current()const{return path_;}
    const std::string& Requested()const{return draft_;}
    const std::vector<Entry>& Rows()const{return rows_;}
private:
    bool Begin(const std::string& path){if(pending_)return false;draft_=path;pending_=true;return true;}
    std::string path_,draft_;
    std::vector<Entry> rows_;
    size_t row_=0;bool pending_=false,truncated_=false;
};
