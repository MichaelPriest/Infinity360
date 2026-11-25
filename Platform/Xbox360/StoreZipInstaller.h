#pragma once
#include "StorePackage.h"
// XEX or bounded ZIP package containing exactly one root default.xex.
bool StoreInstallPackage(const FileWorkspace& workspace,const StoreItem& item,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error);

// A confirmed update preserves the complete old application, restores it on
// a failed install when possible, and never deletes archived versions.
bool StoreInstallManaged(const FileWorkspace& workspace,const StoreItem& item,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error);
