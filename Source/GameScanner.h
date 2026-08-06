#pragma once

#include <string>
#include <vector>

struct Game {
    std::string name;
    std::string executable;
    std::string device;
    bool favorite;
};

class GameScanner {
public:
    explicit GameScanner(std::vector<std::string> roots = DefaultRoots());
    std::vector<Game> Scan() const;
    static std::vector<std::string> DefaultRoots();

private:
    std::vector<std::string> roots_;
};
