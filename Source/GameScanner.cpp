#include "GameScanner.h"

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
#ifdef _XBOX
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
        if (!ec) entries.push_back(entry);
    }
    if (ec) return false;
#endif
    std::sort(entries.begin(), entries.end(), entryLess);
    return true;
}
GameScanner::GameScanner(const std::vector<std::string>& roots) : roots_(roots) {}

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
        const std::string candidate=compactRoot(defaults[i]);
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
        const std::string candidate=compactRoot(line);
        if(candidate.empty() || candidate[0]=='#' || !allowedRoot(candidate)) continue;
        if(seen.insert(canonicalKey(candidate)).second)
            roots.push_back(candidate);
    }
    return roots;
}

std::vector<Game> GameScanner::Scan() const {
    std::vector<Game> games;
#ifdef _XBOX
    // The Xbox 360 XDK predates std::filesystem. Use the Win32-compatible
    // directory API exposed by XTL and keep the public scanner API portable.
    std::vector<std::string> pending = roots_;
    while (!pending.empty()) {
        const std::string directory = pending.back();
        pending.pop_back();
        std::vector<FolderEntry> entries;
        ReadFolder(directory, entries);
        for (size_t i=0; i<entries.size(); ++i) {
            const FolderEntry& entry=entries[i];
            const std::string path=directory+"\\"+entry.name;
            if (entry.directory) pending.push_back(path);
            else if (_stricmp(entry.name.c_str(), "default.xex") == 0) {
                const size_t sep=directory.find_last_of("\\/");
                Game game={directory.substr(sep+1),path,directory.substr(0,directory.find_first_of("\\/")),false};
                games.push_back(game);
            }
        }
    }
#else
    for (const std::string& root : roots_) {
        std::error_code error;
        if (!fs::exists(root, error)) continue;
        fs::recursive_directory_iterator it(root, fs::directory_options::skip_permission_denied, error);
        const fs::recursive_directory_iterator end;
        while (it != end) {
            if (error) { error.clear(); it.increment(error); continue; }
            const fs::directory_entry entry = *it;
            if (entry.is_regular_file(error) && entry.path().filename() == "default.xex") {
                const fs::path folder = entry.path().parent_path();
                games.push_back({folder.filename().string(), entry.path().string(), root, false});
                it.disable_recursion_pending();
            }
            it.increment(error);
        }
    }
#endif
    std::sort(games.begin(), games.end(), gameLess);
    return games;
}
