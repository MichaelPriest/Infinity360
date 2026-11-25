#pragma once
#include "StorePackage.h"
#include <functional>
// Downloads only explicitly authorized store packages. Callback returns false
// on cancellation/display failure. A successful result is verified staging.
std::string XsdkDownload(const StoreItem& item,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,bool& success);
