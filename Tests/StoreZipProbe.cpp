#include "StoreZipInstaller.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
int main(int argc,char** argv){
    if(argc!=5)return 2;
    namespace fs=std::filesystem;
    const std::string root=std::string(argv[1])+"/";
    StoreItem item{};item.id="fixture";item.category="homebrew";item.rights="synthetic test";item.sha256=argv[2];
    item.bytes=fs::file_size(fs::path(root)/"UserFiles/Store/fixture.pkg");
    FileWorkspace workspace(root);
    const bool expected=std::string(argv[3])=="ok",cancel=std::string(argv[4])=="cancel";
    std::string error;
    bool success=StoreInstallPackage(workspace,item,root,[&](unsigned long long done,unsigned long long total){return !cancel||done<total/2;},error);
    if(success!=expected){std::cerr<<error;return 1;}
    StoreCatalog cat(root);
    if(expected){
        assert(cat.Installed(item));
        assert(fs::exists(fs::path(root)/"UserFiles/Homebrew/fixture/data/resource.bin"));
        assert(!StoreInstallPackage(workspace,item,root,[](auto,auto){return true;},error));
    }else assert(!cat.Installed(item));
    for(auto& entry:fs::recursive_directory_iterator(root))assert(entry.path().filename().string().find("AXCOPY-DISC-")!=0);
    return 0;
}
