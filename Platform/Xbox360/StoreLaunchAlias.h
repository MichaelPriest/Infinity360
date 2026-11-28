#pragma once
#include "StoreCatalog.h"
namespace StoreLaunchAlias {
inline std::string Entry(const StoreItem& item){
    static const struct {const char* id;const char* sha;const char* name;} pins[]={
        {"swanstation","9eae76effddb10a1c1b4c7e5f852b64018842b01eea7c00a3c31c993a51a0d25","swanstation.xex"},
        {"dusklight","8d74eacc232e3bb21ce7776e1d58eb4fccdb27cedff9ea1f55b40c38ca0a70cf","dusklight.xex"},
        {"starship","b41c2d8ce53ad5d8386b47c5042114c8b14cc04d0ebd922fb4799c514244eed6","starship.xex"},
        {"homebrew-app-store","44796afc62b577fbb9be65359e0a1de97d6ea794869fae645983bfc866a16958","homebrew_app_store.xex"},
    };
    if(item.provider.empty()&&(item.category=="app"||item.category=="homebrew"||item.category=="emulator"))
        for(const auto& p:pins)if(item.id==p.id&&item.sha256==p.sha)return p.name;
    return "default.xex";
}
}
