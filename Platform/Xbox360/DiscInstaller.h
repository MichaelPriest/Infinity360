#pragma once
#include "FileWorkspace.h"
#include "DiscIdentity.h"
#include <functional>
// Copies only the filesystem already exposed by the console's optical driver.
bool InstallDiscFiles(const FileWorkspace& workspace,const std::string& appRoot,
    const std::string& source,const std::string& mount,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,
    std::string& installedPath,std::string& error,const XboxExecutionInfo* expected=nullptr);

struct DiscInstallRequest {
    enum class Format { Extracted, God };
    Format format = Format::Extracted;
    std::string mount;
    XboxExecutionInfo identity{};
};

bool InstallDiscGod(const FileWorkspace& workspace, const std::string& appRoot,
    const std::string& source, const std::string& mount,
    const std::function<bool(unsigned long long, unsigned long long)>& progress,
    std::string& installedPath, std::string& error, const XboxExecutionInfo& expected);
