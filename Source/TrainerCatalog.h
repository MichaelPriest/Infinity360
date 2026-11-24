#pragma once
#include <string>
#include <vector>
#include "ContentCatalog.h"

struct TrainerEntry {
    std::string name, id, launcher;
    unsigned titleId, mediaId, version;
    enum Match { Unknown, DifferentMedia, DifferentVersion, HeaderMatches };
    Match Compatibility(const XboxExecutionInfo& game) const;
};

class TrainerCatalog {
public:
    static std::vector<TrainerEntry> Scan(const std::string& appDirectory,unsigned titleId);
    // Re-read the manifest and launcher immediately before handing it to XLaunchNewImage.
    static bool Recheck(const std::string& appDirectory,const TrainerEntry& entry);
};
