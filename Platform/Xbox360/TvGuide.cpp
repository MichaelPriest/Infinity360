#include "TvGuide.h"
#include "third_party/yxml/yxml.h"
#include <algorithm>
#include <cstdio>
#include <cstring>

namespace {
int field(const std::string& s,size_t pos,size_t digits) {
    int value=0;
    if(pos+digits>s.size()) return -1;
    for(size_t i=pos;i<pos+digits;++i) {
        if(s[i]<'0' || s[i]>'9') return -1;
        value=value*10+s[i]-'0';
    }
    return value;
}
int leaps(int year) { return year/4-year/100+year/400; }
int64_t timestamp(const std::string& s) {
    const size_t zone=s.find_first_of(" \t");
    const size_t len=zone==std::string::npos?s.size():zone;
    if(len!=12 && len!=14) return -1;
    const int year=field(s,0,4),month=field(s,4,2),day=field(s,6,2);
    const int hour=field(s,8,2),minute=field(s,10,2),sec=len==14?field(s,12,2):0;
    const int md[]={31,28+leaps(year)-leaps(year-1),31,30,31,30,31,31,30,31,30,31};
    if(year<1970 || month<1 || month>12 || day<1 || day>md[month-1] ||
       hour<0 || hour>23 || minute<0 || minute>59 || sec<0 || sec>59) return -1;
    int days=(year-1970)*365+leaps(year-1)-leaps(1969)+day-1;
    for(int i=1;i<month;++i) days+=md[i-1];
    int offset=0;
    const size_t z=s.find_first_not_of(" \t",len);
    if(z!=std::string::npos && s.substr(z)!="UTC" && s.substr(z)!="GMT" && s.substr(z)!="Z") {
        int h=field(s,z+1,2),m=field(s,z+3,2);
        if(z+5!=s.size() || (s[z]!='+' && s[z]!='-') || h<0 || h>23 || m<0 || m>59) return -1;
        offset=(s[z]=='+'?1:-1)*(h*60+m);
    }
    return int64_t(days)*86400+(hour*60+minute-offset)*60+sec;
}
std::string trimmed(const std::string& s) {
    const size_t p=s.find_first_not_of(" \t\r\n");
    return p==std::string::npos?"":s.substr(p,s.find_last_not_of(" \t\r\n")-p+1);
}
bool ordered(const TvProgramme& a,const TvProgramme& b) { return a.start<b.start; }
struct Reader {
    std::string channel,start,stop,title,id,display;
    std::string* attribute;
    int depth,programmeDepth,titleDepth;
    bool inChannel,inDisplay;
    bool root,limited;
    size_t records;
    Reader():attribute(0),depth(0),programmeDepth(0),titleDepth(0),inChannel(false),inDisplay(false),root(false),limited(false),records(0) {}
    void event(yxml_ret_t token,const yxml_t& xml,std::map<std::string,TvGuide::Schedule>& channels,
               std::map<std::string,std::string>& aliases) {
        switch(token) {
        case YXML_ELEMSTART:
            ++depth;
            if(depth==1) root=std::strcmp(xml.elem,"tv")==0;
            if(root && depth==2 && !std::strcmp(xml.elem,"channel")) {inChannel=true;id.clear();}
            if(inChannel && depth==3 && !std::strcmp(xml.elem,"display-name")) {inDisplay=true;display.clear();}
            if(root && depth==2 && !std::strcmp(xml.elem,"programme")) {
                programmeDepth=depth;channel.clear();start.clear();stop.clear();title.clear();
            } else if(programmeDepth && depth==3 && title.empty() && !std::strcmp(xml.elem,"title")) titleDepth=depth;
            break;
        case YXML_ATTRSTART:
            if(inChannel && depth==2 && !std::strcmp(xml.attr,"id")) attribute=&id;
            if(programmeDepth && depth==programmeDepth) {
                if(!std::strcmp(xml.attr,"channel")) attribute=&channel;
                else if(!std::strcmp(xml.attr,"start")) attribute=&start;
                else if(!std::strcmp(xml.attr,"stop")) attribute=&stop;
            }
            break;
        case YXML_ATTRVAL:
            if(attribute && attribute->size()<256) attribute->append(xml.data);
            break;
        case YXML_ATTREND: attribute=0;break;
        case YXML_CONTENT:
            if(inDisplay && display.size()<256) display.append(xml.data);
            if(titleDepth && title.size()<256) title.append(xml.data);
            break;
        case YXML_ELEMEND:
            if(inDisplay && depth==3) {
                inDisplay=false;const std::string name=trimmed(display);
                if(!id.empty() && !name.empty() && aliases.size()<65536) {
                    std::pair<std::map<std::string,std::string>::iterator,bool> found=aliases.insert(std::make_pair(name,id));
                    if(!found.second && found.first->second!=id) found.first->second.clear();
                }
            }
            if(inChannel && depth==2) inChannel=false;
            if(depth==titleDepth) titleDepth=0;
            if(depth==programmeDepth) {
                TvProgramme show;
                show.start=timestamp(start);show.stop=stop.empty()?0:timestamp(stop);
                if(!channel.empty() && show.start>=0 && (stop.empty() || show.stop>show.start)) {
                    if(records<65536) {
                        show.title=trimmed(title);if(show.title.empty()) show.title="Sem título";
                        channels[channel].push_back(show);++records;
                    } else limited=true;
                }
                programmeDepth=0;
            }
            --depth;break;
        default:break;
        }
    }
};
}
TvGuide::Status TvGuide::Load(const std::string& path) {
    channels_.clear();aliases_.clear();status_=Absent;
    FILE* f=std::fopen(path.c_str(),"rb");if(!f) return status_;
    Reader reader;char stack[4096];yxml_t xml;yxml_init(&xml,stack,sizeof stack);
    unsigned char buf[8192];size_t n;bool valid=true;
    while(valid && (n=std::fread(buf,1,sizeof buf,f))!=0) {
        for(size_t i=0;i<n;++i) {
            const yxml_ret_t token=yxml_parse(&xml,buf[i]);
            if(token<0) {valid=false;break;}
            if(token>0) reader.event(token,xml,channels_,aliases_);
        }
    }
    valid=valid && reader.root && !std::ferror(f) && yxml_eof(&xml)>=0;
    std::fclose(f);
    if(!valid) {channels_.clear();aliases_.clear();return status_=Invalid;}
    for(std::map<std::string,Schedule>::iterator it=channels_.begin();it!=channels_.end();++it) {
        Schedule& list=it->second;std::stable_sort(list.begin(),list.end(),ordered);
        for(size_t i=0;i+1<list.size();++i) if(!list[i].stop) list[i].stop=list[i+1].start;
    }
    return status_=reader.limited?Limited:Ready;
}
const TvGuide::Schedule& TvGuide::For(const std::string& id,const std::string& fallback) const {
    static const Schedule empty;
    std::map<std::string,Schedule>::const_iterator it=channels_.find(id);
    if(it==channels_.end()) it=channels_.find(fallback);
    if(it==channels_.end()) {
        std::map<std::string,std::string>::const_iterator alias=aliases_.find(fallback);
        if(alias!=aliases_.end() && !alias->second.empty()) it=channels_.find(alias->second);
    }
    return it==channels_.end()?empty:it->second;
}
const TvProgramme* TvGuide::Current(const Schedule& list,int64_t now) {
    TvProgramme point;point.start=now;
    Schedule::const_iterator it=std::upper_bound(list.begin(),list.end(),point,ordered);
    while(it!=list.begin()) {--it;if(it->stop>now) return &*it;}
    return 0;
}
const TvProgramme* TvGuide::Next(const Schedule& list,int64_t now) {
    TvProgramme point;point.start=now;
    Schedule::const_iterator it=std::upper_bound(list.begin(),list.end(),point,ordered);
    return it==list.end()?0:&*it;
}
