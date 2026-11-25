#include "PackageImport.h"
#include <filesystem>
#include <cassert>
namespace fs=std::filesystem;
int main(){
    auto base=fs::temp_directory_path()/"infinity-package-import-fixture";fs::remove_all(base);
    fs::create_directories(base/"UserFiles/PackageImport");std::string root=base.string()+"/";
    const std::string name="0123456789ABCDEF";auto source=base/"UserFiles/PackageImport"/name;
    std::string raw(0x1711,'\0');raw.replace(0,4,"LIVE");
    auto put=[&](size_t at,unsigned v){for(int i=3;i>=0;--i){raw[at+i]=static_cast<char>(v&255);v>>=8;}};
    put(0x344,0x7000);put(0x360,0x4d5307e6);put(0x39d,2);put(0x3a9,1);
    {std::ofstream f(source,std::ios::binary);f.write(raw.data(),raw.size());}
    {std::ofstream f(base/"infinity360-package-import.txt");f<<name<<"\n";}
    fs::create_directory(source.string()+".data");std::ofstream(source.string()+".data/Data0000")<<"one";
    FileWorkspace ws(root);PackageLaunch::Ticket ticket;std::string error;
    assert(!PackageImport::Read(ws,root,ticket,error));std::ofstream(source.string()+".data/Data0001")<<"two";
    assert(PackageImport::Read(ws,root,ticket,error));auto go=[](auto,auto){return true;};
    assert(!PackageImport::Install(ws,root,ticket,go,error));
    {std::ofstream f(base/"enable-package-import.txt");f<<"ENABLE-PACKAGE-IMPORT\n";}
    assert(!PackageImport::Install(ws,root,ticket,[](auto done,auto){return done==0;},error));
    assert(PackageImport::Roots(root).empty());
    assert(PackageImport::Install(ws,root,ticket,go,error));
    assert(PackageImport::Roots(root).size()==1);
    auto packages=ContentCatalog(PackageImport::Roots(root)).Scan();assert(packages.size()==1);
    assert(ContentCatalog::CheckPackage(packages[0].path).status==XboxPackageCheck::StructurePresent);
    assert(fs::exists(source));assert(!PackageImport::Install(ws,root,ticket,go,error));
    fs::remove_all(base);
}
