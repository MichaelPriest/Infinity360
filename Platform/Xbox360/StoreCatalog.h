#pragma once
#include <string>
#include <vector>
struct StoreItem {
    std::string id, category, name, version, sha256, url, rights;
    unsigned long long bytes;
};
class StoreCatalog {
public:
    explicit StoreCatalog(const std::string& root):root_(root){}
    bool Load();
    const std::vector<StoreItem>& Items() const {return items_;}
    bool Request(const StoreItem& item);
    bool Cancel(const StoreItem& item);
    bool Requested(const StoreItem& item) const;
    bool Staged(const StoreItem& item) const;
private:
    std::string root_;
    std::vector<StoreItem> items_;
    std::vector<std::string> requests_;
    bool validRequests_;
    bool SaveRequests();
};
