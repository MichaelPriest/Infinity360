#include "ContentCatalog.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs=std::filesystem;

static void put32(std::vector<unsigned char>& bytes,size_t at,unsigned value) {
    for(int i=3;i>=0;--i) {bytes[at+i]=static_cast<unsigned char>(value&255u);value>>=8;}
}
static fs::path makePackage(const fs::path& base,const std::string& titleId,
                            const std::string& type,const std::string& filename,
                            unsigned id,unsigned contentType,const std::string& title) {
    const fs::path directory=base/titleId/type;
    fs::create_directories(directory);
    std::vector<unsigned char> bytes(0x1711u,0);
    bytes[0]='L';bytes[1]='I';bytes[2]='V';bytes[3]='E';
    put32(bytes,0x344u,contentType);
    put32(bytes,0x348u,1);
    put32(bytes,0x354u,0x12345678u);
    put32(bytes,0x360u,id);
    put32(bytes,0x3a9u,0);
    put32(bytes,0x34cu,0);
    put32(bytes,0x350u,4096u);
    for(size_t i=0;i<title.size() && i<0x7fu;++i)
        bytes[0x411u+2*i+1]=static_cast<unsigned char>(title[i]);
    const fs::path path=directory/filename;
    std::ofstream file(path,std::ios::binary);
    file.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
    return path;
}
int main() {
    const fs::path root=fs::temp_directory_path()/
        ("aurorax-content-"+std::to_string(
          std::chrono::steady_clock::now().time_since_epoch().count()));
    const fs::path contentRoot=root/"Content"/"0000000000000000";
    const fs::path god=makePackage(contentRoot,"4D5307E6","00007000",
        "ABCD1234",0x4d5307e6u,0x7000u,"Halo GOD");
    const fs::path xbla=makePackage(contentRoot,"58410901","000D0000",
        "CDEF1234",0x58410901u,0xd0000u,"Arcade Example");
    makePackage(contentRoot,"00000001","00007000",
        "WrongID",0x4d5307e6u,0x7000u,"Should Not Appear");
    makePackage(contentRoot,"58410901","00000002",
        "DLC123",0x58410901u,2u,"Not A Game");
    fs::create_directories(contentRoot/"58410901"/"000D0000"/"ZDATA.data");
    XboxContentPackage result;
    assert(ContentCatalog::ReadHeader(god.string(),result));
    assert(result.name=="Halo GOD" && result.titleId==0x4d5307e6u);
    assert(result.contentType==0x7000u && result.sizeBytes==4096u);
    assert(result.mediaId==0x12345678u);
    // Second language slot and accented UTF-16BE names.
    {
        std::fstream file(xbla,std::ios::binary|std::ios::in|std::ios::out);
        assert(file);
        const char clear[2]={0,0};
        file.seekp(0x411u); file.write(clear,2); // first locale becomes empty
        file.seekp(0x511u);                    // language 2, UTF-16BE
        const unsigned char cafe[]={0,0x43,0,0x61,0,0x66,0,0xE9,0,0};
        file.write(reinterpret_cast<const char*>(cafe),sizeof(cafe));
    }
    assert(ContentCatalog::ReadHeader(xbla.string(),result));
    assert(result.name==std::string("Caf\xC3\xA9"));
    const auto roots=ContentCatalog::DefaultRoots(root.string());
    assert(roots.size()>=1u);
    const auto packages=ContentCatalog(roots).Scan();
    assert(packages.size()==2u);
    assert(packages[0].path==god.string());
    assert(packages[1].path==xbla.string());
    assert(packages[1].name==std::string("Caf\xC3\xA9"));
    const fs::path invalid=root/"bad_package";
    std::ofstream(invalid,std::ios::binary)<<"This is not an STFS file";
    assert(!ContentCatalog::ReadHeader(invalid.string(),result));
    assert(!ContentCatalog::ReadHeader((root/"missing").string(),result));
    fs::remove_all(root);
}
