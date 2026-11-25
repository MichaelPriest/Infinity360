#include "ContentBackup.h"
#include "ArtImport.h"
#include <filesystem>
#include <fstream>
#include <cassert>
namespace fs=std::filesystem;
int main(){
    const auto base=fs::temp_directory_path()/"infinity-content-backup-fixture";
    fs::remove_all(base);fs::create_directories(base/"UserFiles/TitleUpdates/494E4635");
    std::string root=base.string()+"/",raw(0x1800,'\0');raw.replace(0,4,"LIVE");
    auto put=[&](size_t at,unsigned value){for(int i=3;i>=0;--i){raw[at+i]=static_cast<char>(value&255);value>>=8;}};
    put(0x344,0xb0000);put(0x360,0x494E4635);put(0x354,0x11223344);
    const auto source=base/"UserFiles/TitleUpdates/494E4635/update.tu";
    {std::ofstream file(source,std::ios::binary);file.write(raw.data(),raw.size());}
    FileWorkspace ws(root);auto rows=ContentBackup::Staged(ws,root,0x494E4635);assert(rows.size()==1);
    std::string error;auto go=[](auto,auto){return true;};
    assert(!ContentBackup::Archive(ws,root,rows[0],go,error));
    {std::ofstream flag(base/"enable-content-backup.txt");flag<<"ENABLE-CONTENT-BACKUP\n";}
    assert(!ContentBackup::Archive(ws,root,rows[0],[](auto,auto){return false;},error));
    assert(ContentBackup::Archive(ws,root,rows[0],go,error));
    assert(fs::exists(source));assert(fs::file_size(base/"UserFiles/ContentBackups/494E4635/000B0000-0000/content.bin")==raw.size());
    assert(ContentBackup::Archive(ws,root,rows[0],go,error));
    std::vector<unsigned char> rgba;assert(!ArtImport::Convert("not png",true,rgba));
    const unsigned char png[]={137,80,78,71,13,10,26,10,0,0,0,13,73,72,68,82,0,0,0,1,0,0,0,1,8,6,0,0,0,31,21,196,137,0,0,0,13,73,68,65,84,120,156,99,248,207,192,240,31,0,5,0,1,255,137,153,61,29,0,0,0,0,73,69,78,68,174,66,96,130};
    std::string encoded(reinterpret_cast<const char*>(png),sizeof png);
    assert(ArtImport::Convert(encoded,true,rgba)&&rgba.size()==160*240*4);
    assert(ArtImport::Save(ws,root,0x494E4635,true,encoded,error));
    assert(!ArtImport::Save(ws,root,0x494E4635,true,encoded,error));
    assert(ArtImport::Convert(encoded,false,rgba)&&rgba.size()==640*360*4);
    fs::remove_all(base);
}
