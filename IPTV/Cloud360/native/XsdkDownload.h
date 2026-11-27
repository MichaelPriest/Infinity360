#pragma once
#include "StorePackage.h"
#include "Modules/Scripts/LuaRepository.h"
#include "UnityCatalog.h"
#include "ClockWeather.h"
#include <functional>
// Downloads only explicitly authorized store packages. Callback returns false
// on cancellation/display failure. A successful result is verified staging.
std::string XsdkDownload(const StoreItem& item,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,bool& success);

// Fetch a bounded, explicitly pinned remote catalog; never download its items automatically.
std::string XsdkLuaSync(const InfinityLuaRepo::Manifest& manifest,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,bool& success);

std::string XsdkLuaDownload(const InfinityLuaRepo::Entry& entry,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,bool& success);

std::string XsdkSyncProvider(const StoreProvider& provider,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,bool& success);

std::string XsdkUnityUpdates(const XboxExecutionInfo& info,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,std::vector<UnityUpdate>& updates,bool& success);

std::string XsdkUnityDownload(const XboxExecutionInfo& info,const UnityUpdate& selected,const std::string& root,
    const std::function<bool(unsigned long long,unsigned long long)>& progress,bool& success);

std::string XsdkUnityCover(unsigned title,const std::string& root,const std::function<bool(unsigned long long,unsigned long long)>& progress,bool& success);

std::string XsdkWeather(unsigned city,const std::string& root,const std::function<bool(unsigned long long,unsigned long long)>& progress,ClockWeather::Reading& reading);
