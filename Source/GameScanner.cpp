#include "GameScanner.h"
#ifdef __XSDK__
#include "XsdkPaths.h"
#endif

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#ifdef _XBOX
#include <xtl.h>
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

namespace {
bool entryLess(const FolderEntry& a, const FolderEntry& b) {
    if (a.directory != b.directory) return a.directory;
    return a.name < b.name;
}
bool gameLess(const Game& a, const Game& b) { return a.name < b.name; }

std::string compactRoot(std::string input) {
    const size_t first=input.find_first_not_of(" \t\r");
    if(first==std::string::npos) return "";
    const size_t last=input.find_last_not_of(" \t\r");
    input=input.substr(first,last-first+1);
    while(input.size()>2 && (input[input.size()-1]=='/' || input[input.size()-1]=='\\'))
        input.erase(input.size()-1);
    return input;
}
std::string canonicalKey(std::string root) {
#ifdef __XSDK__
    return XsdkPaths::Key(root);
#elif defined(_XBOX)
    for(size_t i=0;i<root.size();++i) {
        if(root[i]=='/') root[i]='\\';
        root[i]=(char)std::tolower((unsigned char)root[i]);
    }
#endif
    return root;
}
bool allowedRoot(const std::string& name) {
    if(name.size()<3 || name.size()>220) return false;
    for(size_t i=0;i<name.size();++i) {
        const unsigned char ch=(unsigned char)name[i];
        if(ch<32 || ch==127 || ch=='*' || ch=='?' || ch=='"' || ch=='|'
           || ch=='<' || ch=='>') return false;
    }
    size_t segment=0;
    for(size_t i=0;i<=name.size();++i) if(i==name.size() ||
        name[i]=='/' || name[i]=='\\') {
        const std::string part=name.substr(segment,i-segment);
        if(part=="." || part=="..") return false;
        segment=i+1;
    }
#ifdef _XBOX
    const std::string lower=canonicalKey(name);
    const char* const prefixes[]={"game:\\","hdd1:\\","usb0:\\",
                                   "usb1:\\","mu0:\\","mu1:\\"};
    bool safe=false;
    for(size_t i=0;i<sizeof(prefixes)/sizeof(prefixes[0]);++i)
        if(lower.find(prefixes[i])==0 &&
            lower.size()>std::char_traits<char>::length(prefixes[i]))
            safe=true;
    if(!safe) return false;
#endif
    return true;
}
}

bool ReadFolder(const std::string& path, std::vector<FolderEntry>& entries) {
    entries.clear();
#ifdef _XBOX
    WIN32_FIND_DATAA info;
    const std::string pattern = path + (!path.empty() && path[path.size()-1]=='\\' ? "*" : "\\*");
    HANDLE handle = FindFirstFileA(pattern.c_str(), &info);
    if (handle == INVALID_HANDLE_VALUE) return GetLastError() == ERROR_FILE_NOT_FOUND;
    do {
        FolderEntry entry = {info.cFileName, (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0};
        if (entry.name != "." && entry.name != "..") entries.push_back(entry);
    } while (FindNextFileA(handle, &info));
    FindClose(handle);
#else
    std::error_code ec;
    for (fs::directory_iterator it(path, ec), end; !ec && it!=end; it.increment(ec)) {
        FolderEntry entry = {it->path().filename().string(), it->is_directory(ec)};
        if (!ec && entry.name!="." && entry.name!="..") entries.push_back(entry);
    }
    if (ec) return false;
#endif
    std::sort(entries.begin(), entries.end(), entryLess);
    return true;
}
GameScanner::GameScanner(const std::vector<std::string>& roots) : roots_(roots) {}
std::string GameScanner::ValidRoot(const std::string& path) {
    const std::string candidate=compactRoot(path);
#ifdef __XSDK__
    return allowedRoot(candidate)?XsdkPaths::Normalize(candidate):"";
#else
    return allowedRoot(candidate)?candidate:"";
#endif
}
bool GameScanner::SameRoot(const std::string& a,const std::string& b) {
    return canonicalKey(compactRoot(a))==canonicalKey(compactRoot(b));
}

std::vector<std::string> GameScanner::DefaultRoots() {
#ifdef _XBOX
    // Series360-compatible extracted XEX roots. GOD/Content packages need a
    // separate Xbox content indexer; they are not directly launchable XEX.
    const char* paths[] = {
        "game:\\Games",
        "HDD1:\\Games", "HDD1:\\XBOX 360", "HDD1:\\XBLA",
        "HDD1:\\Xbox Original",
        "USB0:\\Games", "USB0:\\XBOX 360", "USB0:\\XBLA",
        "USB0:\\Xbox Original",
        "USB1:\\Games", "USB1:\\XBOX 360", "USB1:\\XBLA",
        "USB1:\\Xbox Original"
    };
#elif defined(__XSDK__)
    // XSDK resolves these aliases without mounting or changing system links.
    const char* paths[] = {"game:/Games",
        "hdd:/Games","hdd:/XBOX 360","hdd:/XBLA","hdd:/Xbox Original",
        "usb0:/Games","usb0:/XBOX 360","usb0:/XBLA","usb0:/Xbox Original",
        "usb1:/Games","usb1:/XBOX 360","usb1:/XBLA","usb1:/Xbox Original",
        "usb2:/Games","usb2:/XBOX 360","usb2:/XBLA","usb2:/Xbox Original",
        "hdd:/Infinity360Files/Games","usb0:/Infinity360Files/Games",
        "usb1:/Infinity360Files/Games","usb2:/Infinity360Files/Games"};
#else
    const char* paths[] = {"Games", "USB/Games"};
#endif
    return std::vector<std::string>(paths,paths+sizeof(paths)/sizeof(paths[0]));
}

std::vector<std::string> GameScanner::LoadRoots(
    const std::string& configFile,const std::vector<std::string>& defaults) {
    enum { MaxRoots=32, MaxConfigBytes=8192 };
    std::vector<std::string> roots;
    std::set<std::string> seen;
    for(size_t i=0;i<defaults.size() && roots.size()<MaxRoots;++i) {
        const std::string candidate=ValidRoot(defaults[i]);
        if(allowedRoot(candidate) && seen.insert(canonicalKey(candidate)).second)
            roots.push_back(candidate);
    }
    std::ifstream file(configFile.c_str(),std::ios::binary);
    if(!file) return roots;
    std::string line;
    size_t bytes=0;
    while(roots.size()<MaxRoots && std::getline(file,line)) {
        bytes+=line.size()+1;
        if(bytes>MaxConfigBytes || line.size()>255) break;
        const std::string candidate=ValidRoot(line);
        if(candidate.empty() || candidate[0]=='#' || !allowedRoot(candidate)) continue;
        if(seen.insert(canonicalKey(candidate)).second)
            roots.push_back(candidate);
    }
    return roots;
}

std::vector<GameScanner::Depth> GameScanner::LoadDepths(const std::string& file){
    std::vector<Depth> result;
    std::ifstream in(file.c_str(),std::ios::binary);
    char bytes[8193];in.read(bytes,sizeof bytes);
    if(in.bad()||in.gcount()>=static_cast<std::streamsize>(sizeof bytes))return {};
    const std::string data(bytes,static_cast<size_t>(in.gcount()));
    size_t pos=0;
    while(pos<data.size()&&result.size()<32){
        auto end=data.find('\n',pos);if(end==std::string::npos)end=data.size();
        std::string line=data.substr(pos,end-pos);pos=end+1;
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        auto tab=line.find('\t');if(tab==std::string::npos||tab==0||tab>2)continue;
        unsigned levels=0;bool valid=true;
        for(size_t i=0;i<tab;++i){if(line[i]<'0'||line[i]>'9'){valid=false;break;}levels=levels*10+unsigned(line[i]-'0');}
        if(!valid||levels>MaxDepth||line.size()-tab-1>255||line.find('\t',tab+1)!=std::string::npos)continue;
        const auto path=ValidRoot(line.substr(tab+1));if(path.empty())continue;
        bool duplicate=false;for(const auto& d:result)if(SameRoot(d.path,path)){duplicate=true;break;}
        if(!duplicate)result.push_back({path,levels});
    }
    return result;
}
unsigned GameScanner::DepthFor(const std::vector<Depth>& depths,const std::string& root){
    for(const auto& d:depths)if(SameRoot(d.path,root))return std::min(d.levels,MaxDepth);
    return MaxDepth;
}
std::vector<Game> GameScanner::Scan(const std::vector<Depth>& depths) const {
    // Bound work even when removable media contains deeply nested folders.
    // A per-root visited set allows an explicitly configured nested root to
    // receive its own depth budget; executable keys deduplicate across roots.
    struct Pending { std::string path; unsigned depth; };
    const size_t MaxDirectories=32768, MaxGames=16384;
    std::vector<Game> games;
    std::set<std::string> seenGames;
    size_t scanned=0;
    for(size_t r=0;r<roots_.size() && scanned<MaxDirectories && games.size()<MaxGames;++r) {
        const std::string root=ValidRoot(roots_[r]);
        if(root.empty())continue;
        const unsigned maxDepth=DepthFor(depths,root);
        std::vector<Pending> pending;
        Pending first={root,0};pending.push_back(first);
        std::set<std::string> visited;
        while(!pending.empty() && scanned<MaxDirectories && games.size()<MaxGames) {
            const Pending current=pending.back();pending.pop_back();
#ifndef _XBOX
            std::error_code ec;
            const fs::file_status status=fs::symlink_status(current.path,ec);
            if(ec || !fs::is_directory(status))continue;
#endif
            if(!visited.insert(canonicalKey(current.path)).second)continue;
            ++scanned;
            std::vector<FolderEntry> entries;
            if(!ReadFolder(current.path,entries))continue;
            for(size_t i=0;i<entries.size() && games.size()<MaxGames;++i) {
                const FolderEntry& entry=entries[i];
                const char separator=current.path.find('\\')!=std::string::npos?'\\':'/';
                const std::string path=current.path+((!current.path.empty()&&(current.path.back()=='/'||current.path.back()=='\\'))?"":std::string(1,separator))+entry.name;
                if(path.size()>240)continue;
#ifndef _XBOX
                std::error_code ec;
                const fs::file_status status=fs::symlink_status(path,ec);
                if(ec || fs::is_symlink(status))continue;
#endif
                if(entry.directory) {
                    std::string reserved=entry.name;
                    for(char& ch:reserved)ch=static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
                    if(reserved.compare(0,7,"axcopy-")==0)continue;
                    if(current.depth<maxDepth && pending.size()+scanned<MaxDirectories) {
                        Pending child={path,current.depth+1};pending.push_back(child);
                    }
                    continue;
                }
                std::string filename=entry.name;
                for(size_t j=0;j<filename.size();++j)
                    filename[j]=(char)std::tolower((unsigned char)filename[j]);
                if(filename!="default.xex")continue;
#ifndef _XBOX
                if(!fs::is_regular_file(status))continue;
#ifdef __XSDK__
                // libstdc++ treats device-prefixed paths as relative; avoid
                // prepending game: a second time via weakly_canonical.
                if(!seenGames.insert(canonicalKey(path)).second)continue;
#else
                const fs::path absolute=fs::weakly_canonical(path,ec);
                if(ec || !seenGames.insert(absolute.string()).second)continue;
#endif
#ifdef __XSDK__
                const std::string device=root.substr(0,root.find(':')+1);
#else
                const std::string device=root;
#endif
#else
                if(!seenGames.insert(canonicalKey(path)).second)continue;
                const std::string device=current.path.substr(0,current.path.find_first_of("\\/"));
#endif
                const size_t sep=current.path.find_last_of("\\/");
                Game game={current.path.substr(sep==std::string::npos?0:sep+1),path,device,false};
                games.push_back(game);
            }
        }
    }
    std::sort(games.begin(), games.end(), gameLess);
    return games;
}
