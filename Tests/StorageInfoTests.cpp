#include "StorageInfo.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <limits>

int main() {
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/("aurorax-storage-"+std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root);
    const StorageVolume real=StorageInfo::Read(root.string());
    assert(real.available && real.capacity>0);
    assert(real.writableBytes<=real.freeBytes && real.freeBytes<=real.capacity);
    assert(real.UsedBytes()==real.capacity-real.freeBytes);
    assert(real.UsedPercent()<=100);
    const std::string absent=(root/"missing").string();
    assert(!StorageInfo::Read(absent).available);
    assert(!StorageInfo::Read("").available);
    std::ofstream(root/"file")<<"fixture";
    assert(!StorageInfo::Read((root/"file").string()).available);
    const auto volumes=StorageInfo::ReadAll({root.string(),absent});
    assert(volumes.size()==2 && volumes[0].available && !volumes[1].available);
    const StorageVolume half={"test",true,4096,2048,1024};
    assert(half.UsedPercent()==50 && half.UsedBytes()==2048);
    const StorageVolume full={"test",true,std::numeric_limits<unsigned long long>::max(),0,0};
    assert(full.UsedPercent()==100);
    assert(StorageInfo::FormatBytes(0)=="0 B");
    assert(StorageInfo::FormatBytes(1024)=="1.0 KiB");
    assert(StorageInfo::FormatBytes(1073741824ull)=="1.0 GiB");
    fs::remove_all(root);
}
