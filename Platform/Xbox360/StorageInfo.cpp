#include "StorageInfo.h"
#include <iomanip>
#include <sstream>
#ifdef _XBOX
#include <xtl.h>
#else
#include <filesystem>
#endif

unsigned long long StorageVolume::UsedBytes() const {
    return available && freeBytes<=capacity ? capacity-freeBytes : 0;
}
unsigned StorageVolume::UsedPercent() const {
    if(!available || !capacity || freeBytes>capacity)return 0;
    return static_cast<unsigned>(100.0*static_cast<double>(UsedBytes())/
                                 static_cast<double>(capacity));
}
StorageVolume StorageInfo::Read(const std::string& path) {
    StorageVolume v={path,false,0,0,0};
    if(path.empty())return v;
#ifdef _XBOX
    ULARGE_INTEGER writable,total,free;
    if(!GetDiskFreeSpaceExA(path.c_str(),&writable,&total,&free))return v;
    v.capacity=total.QuadPart;
    v.freeBytes=free.QuadPart;
    v.writableBytes=writable.QuadPart;
#else
    std::error_code ec;
    if(!std::filesystem::is_directory(path,ec) || ec)return v;
    const std::filesystem::space_info data=std::filesystem::space(path,ec);
    if(ec)return v;
    v.capacity=data.capacity;
    v.freeBytes=data.free;
    v.writableBytes=data.available;
#endif
    if(!v.capacity || v.freeBytes>v.capacity || v.writableBytes>v.freeBytes) {
        v.capacity=v.freeBytes=v.writableBytes=0;
        return v;
    }
    v.available=true;
    return v;
}
std::vector<StorageVolume> StorageInfo::ReadAll(const std::vector<std::string>& roots) {
    std::vector<StorageVolume> volumes;
    for(size_t i=0;i<roots.size();++i)volumes.push_back(Read(roots[i]));
    return volumes;
}
std::string StorageInfo::FormatBytes(unsigned long long bytes) {
    static const char* const units[]={"B","KiB","MiB","GiB","TiB"};
    double size=static_cast<double>(bytes);
    unsigned unit=0;
    while(size>=1024.0 && unit<4) {size/=1024.0;++unit;}
    std::ostringstream out;
    out<<std::fixed<<std::setprecision(unit?1:0)<<size<<" "<<units[unit];
    return out.str();
}
