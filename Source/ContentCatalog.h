#pragma once
#include <string>
#include <vector>

// Read-only Xbox 360 content index. No content copying, license manipulation,
// extraction, writing, or launch from STFS/GOD containers.
struct XboxContentPackage {
    std::string path, name;
    unsigned titleId, mediaId, contentType;
    unsigned long long sizeBytes;
};

class ContentCatalog {
public:
    explicit ContentCatalog(const std::vector<std::string>& roots);
    static std::vector<std::string> DefaultRoots(const std::string& appDirectory);
    static bool ReadHeader(const std::string& path, XboxContentPackage& out);
    std::vector<XboxContentPackage> Scan() const;
private:
    std::vector<std::string> roots_;
};
