#pragma once
#include "FileWorkspace.h"
#include "ContentCatalog.h"
#include <functional>
namespace ContentBackup {
struct Record { XboxContentPackage package{}; std::string slot,sha256,original; unsigned long long bytes=0; };
bool ReadRecord(const FileWorkspace& ws,const std::string& root,unsigned title,const std::string& slot,Record& out);
std::vector<Record> List(const FileWorkspace& ws,const std::string& root,unsigned title);
bool Verify(const FileWorkspace& ws,const std::string& root,const Record& selected,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error);
bool Recover(const FileWorkspace& ws,const std::string& root,const Record& selected,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error);
bool ArchiveStaged(const FileWorkspace& ws,const std::string& root,const XboxContentPackage& selected,std::string& error);
std::vector<XboxContentPackage> Archived(const FileWorkspace& ws,const std::string& root,unsigned title);
bool RestoreArchived(const FileWorkspace& ws,const std::string& root,const XboxContentPackage& selected,std::string& error);
std::vector<XboxContentPackage> Staged(const FileWorkspace& ws,const std::string& root,unsigned title);
bool Archive(const FileWorkspace& ws,const std::string& root,const XboxContentPackage& selected,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error);
}
