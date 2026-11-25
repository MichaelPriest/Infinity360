#pragma once
#include "MediaUrl.h"
#include <fstream>
#include <sstream>
namespace StoreHostPolicy {
inline std::string lower(std::string s){for(size_t i=0;i<s.size();++i)if(s[i]>='A'&&s[i]<='Z')s[i]+='a'-'A';return s;}
inline bool Allowed(const std::string& address,const std::string& root) {
    MediaUrl url;
    if(!url.Parse(address)||!url.secure||url.port!=443||address.find('#')!=std::string::npos)return false;
    // Explicit local allowlist of exact DNS names, no wildcard.
    std::ifstream in((root+"infinity360-store-hosts.txt").c_str(),std::ios::binary);
    char bytes[8193];in.read(bytes,sizeof bytes);
    if(in.bad()||in.gcount()>=static_cast<std::streamsize>(sizeof bytes))return false;
    std::istringstream lines(std::string(bytes,static_cast<size_t>(in.gcount())));
    std::string line;unsigned count=0;bool match=false;
    while(std::getline(lines,line)) {
        if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
        if(line.empty()||line[0]=='#')continue;
        if(++count>32||line.size()>253||line.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-")!=std::string::npos)return false;
        if(lower(line)==lower(url.host))match=true;
    }
    return match;
}
// Only GitHub release downloads may follow a bounded explicit redirect to
// the two known asset hosts; every destination must also be locally allowed.
inline bool ReleaseRedirect(const std::string& origin,const std::string& next,const std::string& root){
    MediaUrl a,b;
    if(!Allowed(origin,root)||!Allowed(next,root)||!a.Parse(origin)||!b.Parse(next))return false;
    if(lower(a.host)!="github.com"||a.target.find("/releases/download/")==std::string::npos)return false;
    const auto host=lower(b.host);return host=="release-assets.githubusercontent.com"||host=="objects.githubusercontent.com";
}
}
