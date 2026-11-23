#pragma once

#include <cstddef>
#include <string>
#include <vector>

// A local, user-editable category of games. Never copies, deletes,
// modifies or installs game packages, saves, or executable files.
struct GameCollection {
    std::string name;
    std::vector<std::string> games; // Local XEX or catalogued package paths.
};

class CollectionStore {
public:
    explicit CollectionStore(const std::string& file);
    bool Load();
    bool Save() const;
    bool Create(const std::string& name);
    bool Rename(size_t index,const std::string& name);
    bool Remove(size_t index);
    bool Toggle(size_t index,const std::string& path);
    bool Contains(size_t index,const std::string& path) const;
    const std::vector<GameCollection>& Items() const { return collections_; }
    static bool ValidName(const std::string& name);
private:
    bool Unique(const std::string& name,size_t skip) const;
    std::string file_;
    std::vector<GameCollection> collections_;
};
