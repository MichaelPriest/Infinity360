#include "ContentCatalog.h"
#include "GameScanner.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <set>
#include <vector>

namespace {
const size_t HeaderLength=0x1711;
const size_t MaxPackages=2048;
const size_t MaxTitles=1024;
const size_t MaxFilesPerTitle=256;

std::string join(const std::string& base,const std::string& child) {
    if(base.empty()) return child;
    char sep=base.find('\\')!=std::string::npos?'\\':'/';
    char last=base[base.size()-1];
    return base+(last=='\\'||last=='/'?"":std::string(1,sep))+child;
}
bool hex8(const std::string& name) {
    if(name.size()!=8) return false;
    for(size_t i=0;i<name.size();++i)
        if(!std::isxdigit((unsigned char)name[i])) return false;
    return true;
}
unsigned hexValue(const std::string& name) {
    unsigned result=0;
    for(size_t i=0;i<name.size();++i) {
        const unsigned char ch=(unsigned char)name[i];
        result=(result<<4) |
            (ch<='9'?unsigned(ch-'0'):unsigned((ch|32)-'a'+10));
    }
    return result;
}
bool safeFile(const std::string& name) {
    if(name.empty() || name.size()>64 || name=="." || name=="..") return false;
    for(size_t i=0;i<name.size();++i) {
        const unsigned char ch=(unsigned char)name[i];
        if(!(std::isalnum(ch) || ch=='.' || ch=='_' || ch=='-')) return false;
    }
    return true;
}
unsigned be32(const unsigned char* p) {
    return (unsigned(p[0])<<24)|(unsigned(p[1])<<16)|
           (unsigned(p[2])<<8)|unsigned(p[3]);
}
unsigned long long be64(const unsigned char* p) {
    return (static_cast<unsigned long long>(be32(p))<<32)|be32(p+4);
}
bool gameType(unsigned contentType) {
    // Xbox game, installed game, Original Xbox, GOD, demo, XBLA, indie.
    return contentType==0x1000u || contentType==0x4000u ||
           contentType==0x5000u || contentType==0x7000u ||
           contentType==0x80000u || contentType==0xd0000u ||
           contentType==0x2000000u;
}
void utf8(std::string& output,unsigned codepoint) {
    if(codepoint<0x20u || codepoint==0x7fu) return;
    if(codepoint<0x80u) output+=(char)codepoint;
    else if(codepoint<0x800u) {
        output+=(char)(0xc0u|(codepoint>>6));
        output+=(char)(0x80u|(codepoint&63u));
    } else {
        output+=(char)(0xe0u|(codepoint>>12));
        output+=(char)(0x80u|((codepoint>>6)&63u));
        output+=(char)(0x80u|(codepoint&63u));
    }
}
std::string titleString(const unsigned char* text,size_t bytes) {
    std::string value;
    // Field is often UTF-16BE in real STFS, though early documentation
    // describes the locale field as UTF-8. Accept both conservatively.
    const bool wide=bytes>=4 && text[0]==0 && text[1]!=0;
    if(wide) {
        for(size_t i=0;i+1<bytes && value.size()<120u;i+=2) {
            unsigned cp=(unsigned(text[i])<<8)|text[i+1];
            if(cp==0) break;
            if(cp>=0xd800u && cp<=0xdfffu) continue;
            utf8(value,cp);
        }
    } else {
        for(size_t i=0;i<bytes && value.size()<120u;++i) {
            unsigned char ch=text[i];
            if(!ch) break;
            if(ch>=32u && ch!=127u) value+=(char)ch;
        }
    }
    return value;
}
}

ContentCatalog::ContentCatalog(const std::vector<std::string>& roots):roots_(roots) {}

std::vector<std::string> ContentCatalog::DefaultRoots(
    const std::string& appDirectory) {
    std::vector<std::string> roots;
    roots.push_back(join(join(appDirectory,"Content"),"0000000000000000"));
#ifdef _XBOX
    roots.push_back("HDD1:\\Content\\0000000000000000");
    roots.push_back("USB0:\\Content\\0000000000000000");
    roots.push_back("USB1:\\Content\\0000000000000000");
#endif
    return roots;
}

bool ContentCatalog::ReadHeader(const std::string& path,XboxContentPackage& out) {
    std::ifstream file(path.c_str(),std::ios::binary);
    if(!file) return false;
    unsigned char bytes[HeaderLength];
    file.read(reinterpret_cast<char*>(bytes),sizeof(bytes));
    if(file.gcount()!=static_cast<std::streamsize>(sizeof(bytes))) return false;
    if(std::memcmp(bytes,"CON ",4)!=0 &&
       std::memcmp(bytes,"LIVE",4)!=0 &&
       std::memcmp(bytes,"PIRS",4)!=0) return false;
    const unsigned titleId=be32(bytes+0x360);
    const unsigned type=be32(bytes+0x344);
    if(!titleId || !gameType(type)) return false;
    const unsigned descriptor=be32(bytes+0x3a9);
    if(descriptor>1u) return false;
    out=XboxContentPackage();
    out.titleId=titleId;
    out.mediaId=be32(bytes+0x354);
    out.contentType=type;
    out.sizeBytes=be64(bytes+0x34c);
    out.name=titleString(bytes+0x411,0x80);
    if(out.name.empty()) out.name=titleString(bytes+0x1691,0x80);
    if(out.name.empty()) {
        static const char hex[]="0123456789ABCDEF";
        out.name="Jogo ";
        for(int i=7;i>=0;--i) out.name+=hex[(titleId>>(i*4))&15];
    }
    out.path=path;
    return true;
}

std::vector<XboxContentPackage> ContentCatalog::Scan() const {
    std::vector<XboxContentPackage> packages;
    std::set<std::string> found;
    for(size_t r=0;r<roots_.size() && packages.size()<MaxPackages;++r) {
        std::vector<FolderEntry> titles;
        if(!ReadFolder(roots_[r],titles)) continue;
        for(size_t i=0;i<titles.size() && i<MaxTitles &&
            packages.size()<MaxPackages;++i) {
            if(!titles[i].directory || !hex8(titles[i].name)) continue;
            const unsigned expectedId=hexValue(titles[i].name);
            const std::string titleDir=join(roots_[r],titles[i].name);
            std::vector<FolderEntry> types;
            if(!ReadFolder(titleDir,types)) continue;
            for(size_t t=0;t<types.size() && t<32 && packages.size()<MaxPackages;++t) {
                if(!types[t].directory || !hex8(types[t].name)) continue;
                const unsigned wantedType=hexValue(types[t].name);
                if(!gameType(wantedType)) continue;
                const std::string typeDir=join(titleDir,types[t].name);
                std::vector<FolderEntry> contents;
                if(!ReadFolder(typeDir,contents)) continue;
                for(size_t k=0;k<contents.size() && k<MaxFilesPerTitle &&
                    packages.size()<MaxPackages;++k) {
                    if(contents[k].directory || !safeFile(contents[k].name)) continue;
                    XboxContentPackage entry;
                    if(!ReadHeader(join(typeDir,contents[k].name),entry)) continue;
                    if(entry.contentType!=wantedType || entry.titleId!=expectedId) continue;
                    if(found.insert(entry.path).second) packages.push_back(entry);
                }
            }
        }
    }
    return packages;
}
