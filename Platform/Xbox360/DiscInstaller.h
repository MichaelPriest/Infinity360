#pragma once
#include "FileWorkspace.h"
#include <functional>
// Copies only the filesystem already exposed by the console's optical driver.
// Does not authenticate discs, decrypt sectors, patch licenses or create GOD.
bool InstallDiscFiles(const FileWorkspace& workspace,const std::string& appRoot,
    const std::string& source,const std::string& mount,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,
    std::string& installedPath,std::string& error);
