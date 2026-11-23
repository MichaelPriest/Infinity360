#include "GameScanner.h"

#include <algorithm>
#include <cctype>
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
    const char* paths[] = {"game:\\Games", "HDD1:\\Games", "USB0:\\Games", "USB1:\\Games"};
#else
    const char* paths[] = {"Games", "USB/Games"};
#endif
    return std::vector<std::string>(paths,paths+sizeof(paths)/sizeof(paths[0]));
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
