#include "ContentCatalog.h"
#include "PackageLaunch.h"

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
    const XboxTitleMetadata godMeta=ContentCatalog::DescribeTitle(god.string(),true);
    assert(godMeta.known && godMeta.package && godMeta.titleId==0x4d5307e6u);
    assert(godMeta.contentType==0x7000u && godMeta.declaredBytes==4096u);
    assert(!ContentCatalog::DescribeTitle(god.string(),false).known);
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
    assert(ContentCatalog::CheckPackage(invalid.string()).status==XboxPackageCheck::InvalidHeader);
    assert(ContentCatalog::CheckPackage(xbla.string()).status==XboxPackageCheck::StructurePresent);
    // SVOD header with two data fragments, checked without modifying the package.
    {
        std::fstream file(god,std::ios::binary|std::ios::in|std::ios::out);
        const char two[]={0,0,0,2}, one[]={0,0,0,1};
        file.seekp(0x39d);file.write(two,4);
        file.seekp(0x3a9);file.write(one,4);
    }
    assert(ContentCatalog::CheckPackage(god.string()).status==XboxPackageCheck::MissingFragments);
    const fs::path fragments=god.string()+".data";
    fs::create_directories(fragments);
    std::ofstream(fragments/"Data0000")<<"first";
    XboxPackageCheck partial=ContentCatalog::CheckPackage(god.string());
    assert(partial.status==XboxPackageCheck::MissingFragments && partial.expectedFragments==2 && partial.presentFragments==1);
    std::ofstream(fragments/"Data0001")<<"second";
    assert(ContentCatalog::CheckPackage(god.string()).status==XboxPackageCheck::StructurePresent);
    std::ofstream(fragments/"Data0002")<<"unexpected";
    assert(ContentCatalog::CheckPackage(god.string()).status==XboxPackageCheck::InconsistentFragments);
    fs::remove(fragments/"Data0002");
    std::ofstream(fragments/"Data0001",std::ios::trunc).close();
    assert(ContentCatalog::CheckPackage(god.string()).status==XboxPackageCheck::InconsistentFragments);
    std::ofstream(fragments/"Data0001")<<"second";
    PackageLaunch::Ticket first{},next{};
    assert(PackageLaunch::Capture(god.string(),first));
    assert(PackageLaunch::Capture(xbla.string(),next));
    assert(!PackageLaunch::Same(first,next));
    std::ofstream(fragments/"Data0001",std::ios::app)<<"changed";
    assert(PackageLaunch::Capture(god.string(),next)&&!PackageLaunch::Same(first,next));
    fs::remove(fragments/"Data0001");
    assert(!PackageLaunch::Capture(god.string(),next));
    {std::fstream file(god,std::ios::binary|std::ios::in|std::ios::out);const char one[]={0,0,0,1};file.seekp(0x39d);file.write(one,4);}
    assert(PackageLaunch::Capture(god.string(),next));
    // Read only the XEX2 execution header (no encrypted image is opened).
    const fs::path xex=root/"test.xex";
    std::vector<unsigned char> executable(128,0);
    executable[0]='X';executable[1]='E';executable[2]='X';executable[3]='2';
    put32(executable,8,128);put32(executable,0x14,1);
    put32(executable,24,0x00040006u);put32(executable,28,64);
    put32(executable,64,0x12345678u);
    put32(executable,68,0x20010002u);put32(executable,72,0x20000000u);
    put32(executable,76,0x4d5307e6u);
    executable[82]=1;executable[83]=2;
    {std::ofstream file(xex,std::ios::binary);
     file.write(reinterpret_cast<const char*>(executable.data()),executable.size());}
    XboxExecutionInfo info;
    assert(ContentCatalog::ReadExecutionInfo(xex.string(),info));
    assert(info.titleId==0x4d5307e6u && info.mediaId==0x12345678u);
    assert(info.version==0x20010002u && info.discNumber==1 && info.discCount==2);
    const XboxTitleMetadata xexMeta=ContentCatalog::DescribeTitle(xex.string(),false);
    assert(xexMeta.known && !xexMeta.package && xexMeta.titleId==info.titleId);
    assert(xexMeta.mediaId==info.mediaId && xexMeta.version==info.version);
    assert(xexMeta.baseVersion==0x20000000u && xexMeta.discNumber==1 && xexMeta.discCount==2);
    assert(!ContentCatalog::DescribeTitle(xex.string(),true).known);
    assert(!ContentCatalog::DescribeTitle(invalid.string(),false).known);
    put32(executable,28,0xffffffffu);
    {std::ofstream file(xex,std::ios::binary);
     file.write(reinterpret_cast<const char*>(executable.data()),executable.size());}
    assert(!ContentCatalog::ReadExecutionInfo(xex.string(),info));
    assert(!ContentCatalog::ReadExecutionInfo(god.string(),info));

    // Extras must never appear in the main games list.
    const fs::path tu=makePackage(contentRoot,"4D5307E6","000B0000",
        "tu00000001",0x4d5307e6u,0xb0000u,"Update 1");
    const fs::path dlc=makePackage(contentRoot,"4D5307E6","00000002",
        "DLC0001",0x4d5307e6u,2u,"Expansion");
    const fs::path userRoot=root/"Content"/"E000000000000001";
    const fs::path save=makePackage(userRoot,"4D5307E6","00000001",
        "SAVE1",0x4d5307e6u,1u,"Save 1");
    makePackage(contentRoot,"4D5307E6","000B0000",
        "WRONGID",0x58410901u,0xb0000u,"Wrong title");
    makePackage(userRoot,"4D5307E6","00000002",
        "IGNORED",0x4d5307e6u,2u,"Not shared");
    // Legacy TU in a sibling Cache directory, with a valid matching STFS header.
    fs::create_directories(root/"Cache");
    const auto legacy=root/"Cache"/"TU_16L61V6_0000008000000.00000000000O2";
    fs::copy_file(tu,legacy);
    {std::fstream file(legacy,std::ios::binary|std::ios::in|std::ios::out);
     const char version[]={0,0,0,3};file.seekp(0x358);file.write(version,4);}
    fs::copy_file(dlc,root/"Cache"/"TU_wrong_type");
    fs::copy_file(tu,root/"Cache"/"unrelated");
    fs::copy_file(tu,root/"Cache"/"AXCOPY-part");
    std::ofstream(root/"Cache"/"TU_broken")<<"not STFS";
    fs::copy_file(contentRoot/"4D5307E6"/"000B0000"/"WRONGID",root/"Cache"/"TU_wrong_title");
#ifndef _WIN32
    fs::create_symlink(legacy,root/"Cache"/"TU_link");
#endif
    const auto extras=ContentCatalog(roots).ScanExtras(0x4d5307e6u);
    assert(extras.size()==4);
    bool haveLegacy=false;
    bool haveTu=false,haveDlc=false,haveSave=false;
    for(const auto& item:extras) {
        assert(item.titleId==0x4d5307e6u);
        if(item.path==tu.string() && item.contentType==0xb0000u){haveTu=true;assert(!item.legacyCache);}
        if(item.path==legacy.string()){haveLegacy=true;assert(item.legacyCache&&item.version==3);}
        if(item.path==dlc.string() && item.contentType==2u)haveDlc=true;
        if(item.path==save.string() && item.contentType==1u){haveSave=true;assert(item.saveOwner=="E000000000000001"&&!item.saveDevice.empty());}
    }
    assert(haveTu && haveDlc && haveSave && haveLegacy);
    assert(extras.back().path==tu.string()); // TU versions descending
    assert(ContentCatalog(roots).Scan().size()==2);
    assert(ContentCatalog(roots).ScanExtras(0).empty());

    fs::remove_all(root);
}
