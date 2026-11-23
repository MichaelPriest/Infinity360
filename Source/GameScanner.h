#pragma once

#include <string>
#include <vector>

struct Game {
    std::string name;
    std::string executable;
    std::string device;
    bool favorite;
};

struct FolderEntry {
    std::string name;
    bool directory;
};

bool ReadFolder(const std::string& path, std::vector<FolderEntry>& entries);

class GameScanner {
public:
    explicit GameScanner(const std::vector<std::string>& roots = DefaultRoots());
    std::vector<Game> Scan() const;
    static std::vector<std::string> DefaultRoots();

private:
    std::vector<std::string> roots_;
};
