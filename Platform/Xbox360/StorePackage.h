#pragma once
#include "StoreCatalog.h"
#include "FileWorkspace.h"
#include <stdint.h>
#include <functional>
#include "StoreSha256.h"
class StorePackage {
public:
    explicit StorePackage(const FileWorkspace& workspace);
    ~StorePackage();
    bool Begin(const StoreItem& item,const std::string& root,bool resume=false);
    // Flush and preserve a resumable partial; Cancel explicitly removes it.
    bool Pause();
    bool Write(const unsigned char* bytes,unsigned count);
    bool Finish();
    void Cancel();
    unsigned long long Done() const{return done_;}
    const std::string& Error()const{return error_;}
private:
    StorePackage(const StorePackage&);
    StorePackage& operator=(const StorePackage&);
    const FileWorkspace& workspace_;
    StoreItem item_;
    StoreSha256 hash_;
    std::string folder_,temp_,target_,error_;
    unsigned long long done_;
    bool active_,resumable_;
#if defined(_XBOX) || defined(_WIN32)
    void* handle_;
#else
    int handle_;
#endif
    bool Fail(const char* message);
};
bool StoreNativeSource(const StoreItem& item,const std::string& root);

// Install only standalone, explicitly authorized homebrew/app/emulator XEX.
// Rehash the exact bytes copied; never execute or overwrite a destination.
bool StoreInstallXex(const FileWorkspace& workspace,const StoreItem& item,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error);
