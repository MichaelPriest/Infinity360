#pragma once
#include <string>
#include <vector>

// Read-only Xbox 360 content index. No content copying, license manipulation,
// extraction, writing, or launch from STFS/GOD containers.
struct XboxContentPackage {
    std::string path, name;
    unsigned titleId, mediaId, contentType, version;
    unsigned volumeType, dataFileCount;
    unsigned baseVersion, discNumber, discCount;
    unsigned long long sizeBytes;
    std::string saveOwner,saveDevice; // directory origin only, not authenticated ownership
    bool legacyCache=false; // Inventory origin only; does not imply activation.
};

struct XboxExecutionInfo {
    unsigned titleId, mediaId, version, baseVersion;
    unsigned discNumber, discCount;
};

struct XboxTitleMetadata {
    bool known, package;
    unsigned titleId, mediaId, version, baseVersion, discNumber, discCount;
    unsigned contentType, volumeType;
    unsigned long long declaredBytes;
    XboxTitleMetadata():known(false),package(false),titleId(0),mediaId(0),
        version(0),baseVersion(0),discNumber(0),discCount(0),
        contentType(0),volumeType(0),declaredBytes(0) {}
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
    static XboxTitleMetadata DescribeTitle(const std::string& path,bool package);
    static XboxPackageCheck CheckPackage(const std::string& path);
    static const char* PackageStatus(XboxPackageCheck::Status status);
    // Reads the public XEX2 execution header; does not decrypt the image.
    using ExecutionBytes = unsigned char[24];
    static bool ReadExecutionInfo(const std::string& path, XboxExecutionInfo& out, ExecutionBytes* raw = nullptr);
    std::vector<XboxContentPackage> Scan() const;
    // Extras only, scoped to one title: TU/DLC in common content; saves
    // in profile directories; legacy TU packages in sibling Cache folders.
    // No license, activation or file changes.
    std::vector<XboxContentPackage> ScanExtras(unsigned titleId) const;
private:
    std::vector<std::string> roots_;
};
