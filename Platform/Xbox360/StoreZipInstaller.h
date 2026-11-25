#pragma once
#include "StorePackage.h"
// XEX or bounded ZIP package containing exactly one root default.xex.
bool StoreInstallPackage(const FileWorkspace& workspace,const StoreItem& item,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error);
