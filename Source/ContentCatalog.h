#pragma once
#include <string>
#include <vector>

// Read-only Xbox 360 content index. No content copying, license manipulation,
// extraction, writing, or launch from STFS/GOD containers.
struct XboxContentPackage {
    std::string path, name;
    unsigned titleId, mediaId, contentType, version;
    unsigned volumeType, dataFileCount;
    unsigned long long sizeBytes;
};

struct XboxExecutionInfo {
    unsigned titleId, mediaId, version, baseVersion;
    unsigned discNumber, discCount;
};

struct XboxPackageCheck {
    enum Status { InvalidHeader, NotGame, UnsupportedLayout, MissingFragments,
                  InconsistentFragments, StructurePresent };
    Status status;
    unsigned expectedFragments, presentFragments;
    XboxPackageCheck():status(InvalidHeader),expectedFragments(0),presentFragments(0) {}
};

class ContentCatalog {
public:
    explicit ContentCatalog(const std::vector<std::string>& roots);
    static std::vector<std::string> DefaultRoots(const std::string& appDirectory);
    static bool ReadHeader(const std::string& path, XboxContentPackage& out);
    static XboxPackageCheck CheckPackage(const std::string& path);
    static const char* PackageStatus(XboxPackageCheck::Status status);
    // Reads the public XEX2 execution header; does not decrypt the image.
    static bool ReadExecutionInfo(const std::string& path, XboxExecutionInfo& out);
    std::vector<XboxContentPackage> Scan() const;
    // Extras only, scoped to one title: TU/DLC in common content; saves
    // in profile directories. No license, activation or file changes.
    std::vector<XboxContentPackage> ScanExtras(unsigned titleId) const;
private:
    std::vector<std::string> roots_;
};
