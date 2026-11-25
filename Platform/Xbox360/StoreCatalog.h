#pragma once
#include <string>
#include <vector>
struct StoreItem {
    std::string id, category, name, version, sha256, url, rights;
    unsigned long long bytes;
    unsigned titleId, mediaId, gameVersion; // translation only; zero otherwise
    std::string provider; // empty = legacy local catalog
};
struct StoreProvider {
    std::string id,name,url,sha256;
    unsigned long long bytes=0;
    size_t items=0;
    bool cached=false;
};
struct StoreProgress {
    enum State { Unknown, Receiving, Verified, Failed, NativeVerified, NativePaused };
    State state;
    unsigned long long received;
    StoreProgress():state(Unknown),received(0){}
};
struct StoreQueueSummary {
    size_t queued=0,paused=0,verified=0,unavailable=0;
    unsigned long long remaining=0;
};
class StoreCatalog {
public:
    explicit StoreCatalog(const std::string& root):root_(root){}
    bool Load();
    const std::vector<StoreProvider>& Providers() const{return providers_;}
    bool ProvidersValid() const{return providersValid_;}
    bool ImportProvider(const StoreProvider& provider,const std::string& data,std::string& error);
    const std::vector<StoreItem>& Items() const {return items_;}
    bool Request(const StoreItem& item);
    bool Cancel(const StoreItem& item);
    bool Requested(const StoreItem& item) const;
    bool Staged(const StoreItem& item) const;
    bool Installed(const StoreItem& item) const;
    bool InstalledFolder(const StoreItem& item) const;
    bool PackagePresent(const StoreItem& item) const;
    bool BackupAvailable(const StoreItem& item) const;
    bool ArchiveInstalled(const StoreItem& item,std::string& saved);
    bool RestoreInstalled(const StoreItem& item,std::string& error);
    bool ArchivePackage(const StoreItem& item,std::string& saved);
    bool ListedDownload(const StoreItem& item) const;
    bool DiscardPartial(const StoreItem& item);
    bool QueueNative(const StoreItem& item);
    bool RemoveNative(const StoreItem& item);
    bool PrioritizeNative(const StoreItem& item);
    StoreQueueSummary QueueSummary() const;
    bool NativeQueued(const StoreItem& item) const;
    std::vector<size_t> NativeQueue() const;
    size_t OrphanNativeCount() const;
    bool PruneOrphanNative();
    bool NativeQueueValid() const{return validNative_;}
    StoreProgress Progress(const StoreItem& item) const;
private:
    std::string root_;
    std::vector<StoreItem> items_;
    std::vector<StoreProvider> providers_;
    bool providersValid_=true;
    std::vector<std::string> requests_;
    bool validRequests_=true,validNative_=true;
    std::vector<std::string> native_;
    bool SaveNative();
    bool SaveRequests();
};
