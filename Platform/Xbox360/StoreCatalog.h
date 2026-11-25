#pragma once
#include <string>
#include <vector>
struct StoreItem {
    std::string id, category, name, version, sha256, url, rights;
    unsigned long long bytes;
    unsigned titleId, mediaId, gameVersion; // translation only; zero otherwise
};
struct StoreProgress {
    enum State { Unknown, Receiving, Verified, Failed, NativeVerified, NativePaused };
    State state;
    unsigned long long received;
    StoreProgress():state(Unknown),received(0){}
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
    bool Installed(const StoreItem& item) const;
    bool ListedDownload(const StoreItem& item) const;
    bool DiscardPartial(const StoreItem& item);
    bool QueueNative(const StoreItem& item);
    bool RemoveNative(const StoreItem& item);
    bool NativeQueued(const StoreItem& item) const;
    std::vector<size_t> NativeQueue() const;
    bool NativeQueueValid() const{return validNative_;}
    StoreProgress Progress(const StoreItem& item) const;
private:
    std::string root_;
    std::vector<StoreItem> items_;
    std::vector<std::string> requests_;
    bool validRequests_=true,validNative_=true;
    std::vector<std::string> native_;
    bool SaveNative();
    bool SaveRequests();
};
