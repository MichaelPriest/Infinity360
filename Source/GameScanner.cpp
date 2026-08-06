#include "GameScanner.h"

#include <algorithm>
#include <filesystem>

namespace fs = std::filesystem;

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
    std::sort(games.begin(), games.end(), [](const Game& a, const Game& b) { return a.name < b.name; });
    return games;
}
