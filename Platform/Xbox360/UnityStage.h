#pragma once
#include "UnityCatalog.h"
#include "FileWorkspace.h"
#include <functional>
// Validated TU staging only; no content activation or license modification.
bool UnityStageUpdate(const FileWorkspace& ws,const XboxExecutionInfo& info,const UnityUpdate& item,
    const std::string& expectedMd5,const std::string& root,
    const std::function<bool(const std::function<bool(const unsigned char*,size_t)>&)>& download,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::string& error);
