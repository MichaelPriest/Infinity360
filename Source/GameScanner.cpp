#include "GameScanner.h"

#include <algorithm>
#ifdef _XBOX
#include <xtl.h>
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

GameScanner::GameScanner(std::vector<std::string> roots) : roots_(std::move(roots)) {}

std::vector<std::string> GameScanner::DefaultRoots() {
#ifdef _XBOX
    return {"HDD1:\\Games", "USB0:\\Games", "USB1:\\Games"};
#else
    return {"Games", "USB/Games"};
#endif
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
        WIN32_FIND_DATAA data;
        const std::string pattern = directory + "\\*";
        HANDLE find = FindFirstFileA(pattern.c_str(), &data);
        if (find == INVALID_HANDLE_VALUE) continue;
        do {
            const std::string name = data.cFileName;
            if (name == "." || name == "..") continue;
            const std::string path = directory + "\\" + name;
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                pending.push_back(path);
            } else if (name == "default.xex") {
                const std::string::size_type slash = directory.find_last_of("\\/");
                const std::string title = slash == std::string::npos ? directory : directory.substr(slash + 1);
                games.push_back({title, path, directory.substr(0, directory.find_first_of("\\/")), false});
            }
        } while (FindNextFileA(find, &data));
        FindClose(find);
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
    std::sort(games.begin(), games.end(), [](const Game& a, const Game& b) { return a.name < b.name; });
    return games;
}
