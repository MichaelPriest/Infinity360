#pragma once
#include "FileWorkspace.h"
#include "PackageLaunch.h"
#include <functional>
namespace PackageImport {
bool Read(const FileWorkspace& ws,const std::string& root,PackageLaunch::Ticket& ticket,std::string& error);
std::vector<std::string> Slots(const std::string& personal);
std::vector<std::string> Roots(const std::string& root);
bool Install(const FileWorkspace& ws,const std::string& root,const PackageLaunch::Ticket& expected,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error);
}
