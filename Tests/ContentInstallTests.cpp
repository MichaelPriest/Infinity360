#include "ContentInstall.h"
#include "StoreSha256.h"
#include <filesystem>
#include <fstream>
#include <cassert>
namespace fs=std::filesystem;
int main(){
    const auto base=fs::temp_directory_path()/"infinity-content-install-fixture";fs::remove_all(base);
    fs::create_directories(base/"UserFiles/ContentImport");fs::create_directories(base/"Content/0000000000000000");
    const auto root=base.string()+"/";
    XboxExecutionInfo game{};game.titleId=0x494E4650;game.mediaId=0x11223344;
    std::string error;FileWorkspace ws(root);
    assert(ContentInstall::Filename(0xb0000,"tu00000002_00000000"));
    assert(!ContentInstall::Filename(0xb0000,"TU_123456789"));
    assert(!ContentInstall::Filename(2,"../../test"));
    auto build=[&](unsigned type,const std::string& name,unsigned media){
        std::string raw(0x1800,'\0');raw.replace(0,4,"LIVE");
        auto put=[&](size_t at,unsigned value){for(int i=3;i>=0;--i){raw[at+i]=static_cast<char>(value&255);value>>=8;}};
        put(0x344,type);put(0x360,game.titleId);put(0x354,media);
        auto file=base/"UserFiles/ContentImport"/name;
        {std::ofstream f(file,std::ios::binary);f.write(raw.data(),raw.size());}
        StoreSha256 sha;sha.Update(reinterpret_cast<const unsigned char*>(raw.data()),raw.size());
        {std::ofstream m(base/"infinity360-content-import.tsv");m<<(type==2?"DLC":"TU")<<'\t'<<name<<'\t'<<raw.size()<<'\t'<<sha.Finish()<<'\n';}
    };
    const std::string name="tu00000002_00000000";
    build(0xb0000,name,game.mediaId);
    ContentInstall::Request selected;assert(ContentInstall::Read(ws,root,game,selected,error));
    auto go=[](auto,auto){return true;};
    assert(!ContentInstall::Install(ws,root,game,selected,go,error));
    {std::ofstream marker(base/"enable-content-install.txt");marker<<"ENABLE-CONTENT-INSTALL\n";}
    assert(!ContentInstall::Install(ws,root,game,selected,[](auto done,auto){return done==0;},error));
    assert(ContentInstall::Install(ws,root,game,selected,go,error));
    assert(fs::exists(base/"Content/0000000000000000/494E4650/000B0000"/name));
    assert(fs::exists(base/"UserFiles/ContentImport"/name));
    assert(!ContentInstall::Install(ws,root,game,selected,go,error));
    build(0xb0000,name,0x12345678);assert(!ContentInstall::Read(ws,root,game,selected,error));
    const std::string dlc="0123456789abcdef0123456789abcdef";
    build(2,dlc,0);assert(ContentInstall::Read(ws,root,game,selected,error));
    {std::fstream f(base/"UserFiles/ContentImport"/dlc,std::ios::binary|std::ios::in|std::ios::out);f.seekp(0x17ff);f.put('x');}
    assert(!ContentInstall::Install(ws,root,game,selected,go,error));
    build(2,dlc,0);assert(ContentInstall::Read(ws,root,game,selected,error));
    assert(ContentInstall::Install(ws,root,game,selected,go,error));
    assert(fs::exists(base/"Content/0000000000000000/494E4650/00000002"/dlc));
    // Verify the receipt-based path independently of the manual manifest.
    game.baseVersion=0x10002;
    const auto stage=base/"UserFiles/TitleUpdates/494E4650";fs::create_directories(stage);
    const std::string original="tu00000003_00000000",staged="11223344-42.tu";
    build(0xb0000,original,game.mediaId);
    fs::copy_file(base/"UserFiles/ContentImport"/original,stage/staged);
    std::ifstream data(stage/staged,std::ios::binary);std::string raw((std::istreambuf_iterator<char>(data)),{});
    StoreSha256 sha;sha.Update(reinterpret_cast<const unsigned char*>(raw.data()),raw.size());
    const auto digest=sha.Finish();
    assert(!ContentInstall::ReadStaged(ws,root,game,staged,selected,error));
    {std::ofstream r(stage/(staged+".receipt"));r<<"INFINITY-TU-1\n"<<game.baseVersion<<"\n"<<original<<"\n"<<raw.size()<<"\n"<<digest<<"\n";}
    assert(ContentInstall::ReadStaged(ws,root,game,staged,selected,error));
    auto other=game;other.baseVersion++;
    ContentInstall::Request wrong;assert(!ContentInstall::ReadStaged(ws,root,other,staged,wrong,error));
    assert(!ContentInstall::ReadStaged(ws,root,game,"../"+staged,wrong,error));
    assert(ContentInstall::Install(ws,root,game,selected,go,error));
    assert(fs::exists(base/"Content/0000000000000000/494E4650/000B0000"/original));
    assert(fs::exists(stage/staged));
    assert(!ContentInstall::Install(ws,root,game,selected,go,error));
    for(const auto& e:fs::recursive_directory_iterator(base))assert(e.path().filename()!="AXCOPY-CONTENT.part");
    fs::remove_all(base);
}
