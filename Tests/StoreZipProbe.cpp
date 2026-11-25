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
        const auto installed=fs::path(root)/"UserFiles/Homebrew/fixture";
        std::ofstream(installed/"user-settings.ini")<<"keep me";
        std::string saved;
        assert(cat.ArchiveInstalled(item,saved));assert(!fs::exists(installed));
        assert(fs::exists(fs::path(saved)/"user-settings.ini"));assert(cat.BackupAvailable(item));
        assert(cat.RestoreInstalled(item,error));assert(fs::exists(installed/"user-settings.ini"));
        // Same-size stale catalog is not current when install receipt differs.
        StoreItem newer=item;newer.sha256=std::string(64,'0');
        assert(!cat.Installed(newer));assert(cat.InstalledFolder(newer));
        assert(!StoreInstallManaged(workspace,newer,root,[](auto,auto){return true;},error));
        assert(fs::exists(installed/"user-settings.ini"));
        // Restore an archived version over another active folder, keeping both.
        assert(cat.ArchiveInstalled(item,saved));
        assert(StoreInstallPackage(workspace,item,root,[](auto,auto){return true;},error));
        assert(!fs::exists(installed/"user-settings.ini"));
        assert(cat.RestoreInstalled(item,error));assert(fs::exists(installed/"user-settings.ini"));
        assert(cat.BackupAvailable(item));
        assert(cat.ArchivePackage(item,saved));
        assert(!cat.PackagePresent(item));assert(fs::exists(saved));
    }else assert(!cat.Installed(item));
    for(auto& entry:fs::recursive_directory_iterator(root))assert(entry.path().filename().string().find("AXCOPY-DISC-")!=0);
    return 0;
}
