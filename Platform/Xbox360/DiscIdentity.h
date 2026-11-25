#pragma once
#include "ContentCatalog.h"
namespace DiscIdentity {
inline bool Same(const XboxExecutionInfo& a,const XboxExecutionInfo& b){
    return a.titleId!=0&&a.titleId==b.titleId&&a.mediaId==b.mediaId&&
        a.version==b.version&&a.baseVersion==b.baseVersion&&
        a.discNumber==b.discNumber&&a.discCount==b.discCount;
}
}
