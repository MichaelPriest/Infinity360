#pragma once
#include <string>
#include <vector>

struct StorageVolume {
    std::string path;
    bool available;
    unsigned long long capacity, freeBytes, writableBytes;
    unsigned UsedPercent() const;
    unsigned long long UsedBytes() const;
};

class StorageInfo {
public:
    static StorageVolume Read(const std::string& path);
    static std::vector<StorageVolume> ReadAll(const std::vector<std::string>& roots);
    static std::string FormatBytes(unsigned long long bytes);
};
