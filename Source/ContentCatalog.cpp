#include "ContentCatalog.h"
#include "GameScanner.h"

#include <cctype>
#include <cstring>
#include <cstdio>
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
bool extraType(unsigned type) {
    return type==0x00000001u || type==0x00000002u || type==0x000b0000u;
}
bool profileName(const std::string& name) {
    if(name.size()!=16 || name=="0000000000000000") return false;
    for(size_t i=0;i<name.size();++i)
        if(!std::isxdigit((unsigned char)name[i])) return false;
    return true;
}
void utf8(std::string& output,unsigned codepoint) {
    if(codepoint<0x20u || codepoint==0x7fu || codepoint>0x10ffffu) return;
    if(codepoint<0x80u) output+=(char)codepoint;
    else if(codepoint<0x800u) {
        output+=(char)(0xc0u|(codepoint>>6));
        output+=(char)(0x80u|(codepoint&63u));
    } else if(codepoint<0x10000u) {
        output+=(char)(0xe0u|(codepoint>>12));
        output+=(char)(0x80u|((codepoint>>6)&63u));
        output+=(char)(0x80u|(codepoint&63u));
    } else {
        output+=(char)(0xf0u|(codepoint>>18));
        output+=(char)(0x80u|((codepoint>>12)&63u));
        output+=(char)(0x80u|((codepoint>>6)&63u));
        output+=(char)(0x80u|(codepoint&63u));
    }
}
std::string titleString(const unsigned char* text,size_t bytes) {
    std::string value;
    // Xbox XContent metadata uses UTF-16BE in 9 locale slots.
    for(size_t i=0;i+1<bytes && value.size()<120u;i+=2) {
        unsigned cp=(unsigned(text[i])<<8)|text[i+1];
        if(cp==0u) break;
        if(cp>=0xd800u && cp<=0xdbffu) {
            if(i+3>=bytes) continue;
            const unsigned next=(unsigned(text[i+2])<<8)|text[i+3];
            if(next<0xdc00u || next>0xdfffu) continue;
            cp=0x10000u+((cp-0xd800u)<<10)+(next-0xdc00u);
            i+=2;
        } else if(cp>=0xdc00u && cp<=0xdfffu) continue;
        utf8(value,cp);
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
#elif defined(__XSDK__)
    roots.push_back("hdd:/Content/0000000000000000");
    roots.push_back("usb0:/Content/0000000000000000");
    roots.push_back("usb1:/Content/0000000000000000");
    roots.push_back("usb2:/Content/0000000000000000");
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
    if(!titleId || (!gameType(type) && !extraType(type))) return false;
    const unsigned descriptor=be32(bytes+0x3a9);
    if(descriptor>1u) return false;
    out=XboxContentPackage();
    out.titleId=titleId;
    out.mediaId=be32(bytes+0x354);
    out.version=be32(bytes+0x358);
    out.contentType=type;
    out.volumeType=descriptor;
    out.dataFileCount=be32(bytes+0x39d);
    out.sizeBytes=be64(bytes+0x34c);
    for(size_t locale=0;locale<9 && out.name.empty();++locale)
        out.name=titleString(bytes+0x411+locale*0x100,0x100);
    if(out.name.empty()) out.name=titleString(bytes+0x1691,0x80);
    if(out.name.empty()) {
        static const char hex[]="0123456789ABCDEF";
        out.name=extraType(type)?"Conteudo ":"Jogo ";
        for(int i=7;i>=0;--i) out.name+=hex[(titleId>>(i*4))&15];
    }
    out.path=path;
    return true;
}

XboxTitleMetadata ContentCatalog::DescribeTitle(const std::string& path,bool package) {
    XboxTitleMetadata result;
    result.package=package;
    if(package) {
        XboxContentPackage info;
        if(!ReadHeader(path,info) || !gameType(info.contentType))return result;
        result.titleId=info.titleId;
        result.mediaId=info.mediaId;
        result.version=info.version;
        result.contentType=info.contentType;
        result.volumeType=info.volumeType;
        result.declaredBytes=info.sizeBytes;
    } else {
        XboxExecutionInfo info;
        if(!ReadExecutionInfo(path,info))return result;
        result.titleId=info.titleId;
        result.mediaId=info.mediaId;
        result.version=info.version;
        result.baseVersion=info.baseVersion;
        if(info.discNumber && info.discCount && info.discNumber<=info.discCount) {
            result.discNumber=info.discNumber;
            result.discCount=info.discCount;
        }
    }
    result.known=true;
    return result;
}

const char* ContentCatalog::PackageStatus(XboxPackageCheck::Status status) {
    switch(status) {
    case XboxPackageCheck::NotGame:return "Conteudo nao executavel";
    case XboxPackageCheck::UnsupportedLayout:return "Formato nao suportado";
    case XboxPackageCheck::MissingFragments:return "Fragmentos de dados ausentes";
    case XboxPackageCheck::InconsistentFragments:return "Fragmentos invalidos ou incompletos";
    case XboxPackageCheck::StructurePresent:return "Estrutura presente; launch pendente";
    default:return "Cabecalho invalido ou indisponivel";
    }
}

XboxPackageCheck ContentCatalog::CheckPackage(const std::string& path) {
    XboxPackageCheck check;
    XboxContentPackage package;
    if(!ReadHeader(path,package))return check;
    if(!gameType(package.contentType)) {
        check.status=XboxPackageCheck::NotGame;return check;
    }
    if(package.volumeType==0) {
        // Header-only inspection is not signature, license, or hash validation.
        check.status=package.dataFileCount==0?
            XboxPackageCheck::StructurePresent:XboxPackageCheck::UnsupportedLayout;
        return check;
    }
    // Common multi-file SVOD: a sibling .data directory with Data0000...
    // Do not guess other layouts or pass the package to XLaunchNewImage.
    check.expectedFragments=package.dataFileCount;
    if(package.dataFileCount<=1 || package.dataFileCount>1024) {
        check.status=XboxPackageCheck::UnsupportedLayout;return check;
    }
    const std::string folder=path+".data";
    std::vector<FolderEntry> entries;
    if(!ReadFolder(folder,entries)) {
        check.status=XboxPackageCheck::MissingFragments;return check;
    }
    std::set<unsigned> found;
    bool invalid=false;
    for(size_t i=0;i<entries.size();++i) {
        const std::string& name=entries[i].name;
        if(entries[i].directory || name.size()!=8 || name.compare(0,4,"Data")!=0) {
            invalid=true;continue;
        }
        unsigned index=0;
        bool digits=true;
        for(size_t j=4;j<8;++j) {
            if(name[j]<'0' || name[j]>'9') {digits=false;break;}
            index=index*10+unsigned(name[j]-'0');
        }
        if(!digits || index>=package.dataFileCount || !found.insert(index).second) {
            invalid=true;continue;
        }
        std::ifstream fragment(join(folder,name).c_str(),std::ios::binary|std::ios::ate);
        if(!fragment || fragment.tellg()<=0)invalid=true;
        else ++check.presentFragments;
    }
    check.status=invalid?XboxPackageCheck::InconsistentFragments:
        check.presentFragments==check.expectedFragments?XboxPackageCheck::StructurePresent:
        XboxPackageCheck::MissingFragments;
    return check;
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

bool ContentCatalog::ReadExecutionInfo(const std::string& path,XboxExecutionInfo& out) {
    std::ifstream file(path.c_str(),std::ios::binary);
    if(!file) return false;
    unsigned char header[24];
    if(!file.read(reinterpret_cast<char*>(header),sizeof(header)) ||
       std::memcmp(header,"XEX2",4)!=0) return false;
    const unsigned entries=be32(header+0x14);
    const unsigned imageOffset=be32(header+8);
    if(entries>4096 || imageOffset<24u+entries*8u) return false;
    for(unsigned i=0;i<entries;++i) {
        unsigned char entry[8];
        if(!file.read(reinterpret_cast<char*>(entry),sizeof(entry))) return false;
        if(be32(entry)!=0x00040006u) continue;
        const unsigned at=be32(entry+4);
        if(at<24u+entries*8u || at>imageOffset-24u || at>16u*1024u*1024u)
            return false;
        unsigned char exec[24];
        file.seekg(at,std::ios::beg);
        if(!file.read(reinterpret_cast<char*>(exec),sizeof(exec))) return false;
        XboxExecutionInfo info={};
        info.mediaId=be32(exec);
        info.version=be32(exec+4);
        info.baseVersion=be32(exec+8);
        info.titleId=be32(exec+12);
        info.discNumber=exec[18];info.discCount=exec[19];
        if(!info.titleId) return false;
        out=info;
        return true;
    }
    return false;
}

std::vector<XboxContentPackage> ContentCatalog::ScanExtras(unsigned titleId) const {
    std::vector<XboxContentPackage> result;
    if(!titleId) return result;
    std::set<std::string> seen;
    for(size_t r=0;r<roots_.size() && result.size()<MaxPackages;++r) {
        // Default roots point to the shared 16-zero profile directory.
        // Only traverse its sibling profile directories for saved games.
        std::vector<std::string> owners;
        owners.push_back(roots_[r]);
        const size_t slash=roots_[r].find_last_of("\\/");
        if(slash!=std::string::npos &&
           roots_[r].substr(slash+1)=="0000000000000000") {
            const std::string content=roots_[r].substr(0,slash);
            std::vector<FolderEntry> profiles;
            if(ReadFolder(content,profiles)) {
                for(size_t p=0;p<profiles.size() && owners.size()<65;++p)
                    if(profiles[p].directory && profileName(profiles[p].name))
                        owners.push_back(join(content,profiles[p].name));
            }
        }
        for(size_t o=0;o<owners.size() && result.size()<MaxPackages;++o) {
            // Find case-insensitive TitleID folder on portable filesystems.
            std::vector<FolderEntry> titles;
            if(!ReadFolder(owners[o],titles))continue;
            for(size_t t=0;t<titles.size() && t<MaxTitles && result.size()<MaxPackages;++t) {
                if(!titles[t].directory || !hex8(titles[t].name) ||
                   hexValue(titles[t].name)!=titleId)continue;
                const std::string titleDir=join(owners[o],titles[t].name);
                std::vector<FolderEntry> types;
                if(!ReadFolder(titleDir,types))continue;
                for(size_t k=0;k<types.size() && k<32 && result.size()<MaxPackages;++k) {
                    if(!types[k].directory || !hex8(types[k].name))continue;
                    const unsigned type=hexValue(types[k].name);
                    if(!extraType(type) || (o>0 && type!=1u))continue;
                    const std::string folder=join(titleDir,types[k].name);
                    std::vector<FolderEntry> files;
                    if(!ReadFolder(folder,files))continue;
                    for(size_t i=0;i<files.size() && i<MaxFilesPerTitle &&
                        result.size()<MaxPackages;++i) {
                        if(files[i].directory || !safeFile(files[i].name))continue;
                        XboxContentPackage item;
                        if(ReadHeader(join(folder,files[i].name),item) &&
                           item.titleId==titleId && item.contentType==type &&
                           seen.insert(item.path).second)result.push_back(item);
                    }
                }
            }
        }
    }
    return result;
}
