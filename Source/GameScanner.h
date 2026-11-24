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
    static std::string ValidRoot(const std::string& path);
    static bool SameRoot(const std::string& a,const std::string& b);
    // Merge deduplicated scan roots from a local text file; never modifies disks.
    static std::vector<std::string> LoadRoots(const std::string& configFile,
                                              const std::vector<std::string>& defaults);

private:
    std::vector<std::string> roots_;
};
