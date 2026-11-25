#include "StorePackage.h"
#include <cassert>
#include <filesystem>
#include <fstream>
namespace fs=std::filesystem;
int main(){
    StoreSha256 empty;assert(empty.Finish()=="e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    StoreSha256 abc;abc.Update((const unsigned char*)"a",1);abc.Update((const unsigned char*)"bc",2);
    const std::string digest="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    assert(abc.Finish()==digest);
    StoreSha256 million;std::string block(1000,'a');
    for(int i=0;i<1000;++i)million.Update((const unsigned char*)block.data(),block.size());
    assert(million.Finish()=="cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
    fs::path root=fs::temp_directory_path()/"infinity-store-package-test";
    fs::remove_all(root);fs::create_directories(root);
    std::string base=root.string()+"/";
    FileWorkspace workspace(base);StorePackage out(workspace);
    StoreItem item;item.id="demo";item.bytes=3;item.sha256=digest;item.url="https://example.org/demo";
    assert(!StoreNativeSource(item,base));
    {std::ofstream f(root/"infinity360-store-hosts.txt");f<<"example.org\n";}
    assert(StoreNativeSource(item,base));
    item.url="https://other.example/demo";assert(!StoreNativeSource(item,base));
    assert(out.Begin(item,base));assert(out.Write((const unsigned char*)"bad",3));assert(!out.Finish());
    assert(!fs::exists(root/"UserFiles/Store/demo.pkg"));
    assert(out.Begin(item,base));assert(out.Write((const unsigned char*)"abc",3));assert(out.Finish());
    assert(fs::file_size(root/"UserFiles/Store/demo.pkg")==3);
    StoreCatalog cat(base);assert(cat.Progress(item).state==StoreProgress::NativeVerified);
    assert(!out.Begin(item,base));
    item.id="cancel";assert(out.Begin(item,base));assert(out.Write((const unsigned char*)"a",1));out.Cancel();
    assert(!fs::exists(root/"UserFiles/Store/cancel.pkg"));
    item.id="race";assert(out.Begin(item,base));assert(out.Write((const unsigned char*)"abc",3));
    {std::ofstream f(root/"UserFiles/Store/race.pkg");f<<"original";}
    assert(!out.Finish());assert(fs::file_size(root/"UserFiles/Store/race.pkg")==8);
    for(auto& entry:fs::directory_iterator(root/"UserFiles/Store"))assert(entry.path().extension()!=".part");
    fs::remove_all(root);
}
