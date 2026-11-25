#pragma once
#include "FileWorkspace.h"
#include "ContentCatalog.h"
#include <functional>
namespace ContentBackup {
std::vector<XboxContentPackage> Staged(const FileWorkspace& ws,const std::string& root,unsigned title);
bool Archive(const FileWorkspace& ws,const std::string& root,const XboxContentPackage& selected,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error);
}
