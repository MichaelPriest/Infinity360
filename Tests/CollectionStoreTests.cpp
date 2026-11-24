#include "CollectionStore.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs=std::filesystem;

int main() {
    const fs::path root=fs::temp_directory_path()/
        ("aurorax-collections-"+std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root);
    const std::string location=(root/"aurorax-collections.txt").string();
    const std::string game1="HDD1:\\Games\\Halo 3\\default.xex";
    const std::string game2="USB0:\\XBLA\\Example\\default.xex";

    CollectionStore store(location);
    assert(store.Load());
    assert(store.Items().empty());
    assert(store.Create("Favoritos Retro"));
    assert(!store.Create("favoritos retro"));     // Unique, case insensitive
    assert(store.Create("Series 360"));
    assert(!store.Create("Invalid/Path"));
    assert(!store.Create(" trailing "));
    assert(!store.Create(""));
    assert(!store.Create(std::string(33,'X')));
    assert(store.Toggle(0,game1));
    assert(store.Toggle(0,game2));
    assert(store.Contains(0,game1));
    assert(!store.Toggle(0,"../data\nmalicious")); // Reject delimiter injection
    assert(store.Save());

    CollectionStore restored(location);
    assert(restored.Load());
    assert(restored.Items().size()==2);
    assert(restored.Items()[0].name=="Favoritos Retro");
    assert(restored.Items()[0].games.size()==2);
    assert(restored.Contains(0,game1));
    assert(restored.Rename(0,"Aventura"));
    assert(!restored.Rename(0,"series 360"));
    assert(restored.Toggle(0,game1));             // Remove from collection
    assert(!restored.Contains(0,game1));
    assert(restored.Remove(1));
    assert(restored.Save());

    CollectionStore again(location);
    assert(again.Load());
    assert(again.Items().size()==1);
    assert(again.Items()[0].name=="Aventura");
    assert(again.Items()[0].games.size()==1);
    assert(!again.Contains(0,game1));
    assert(again.Contains(0,game2));
    assert(!again.Remove(99));
    assert(!again.Rename(99,"Missing"));
    assert(!again.Toggle(99,game1));

    // Backup restore after an interrupted FATX rename.
    fs::rename(location,location+".bak");
    CollectionStore recovered(location);
    assert(recovered.Load());
    assert(recovered.Items().size()==1);

    // Corrupt and oversized lines never become executable input.
    {
        std::ofstream out(location,std::ios::binary);
        out<<"# AuroraX collections v1\nC\tOK\nG\tSAFE\n"
           <<"G\tbad\tdata\n"
           <<"C\tBad/Collection\n";
    }
    CollectionStore guarded(location);
    assert(!guarded.Load());
    assert(guarded.Items().size()==1);
    assert(guarded.Items()[0].games.size()==1);

    // Malformed data remains inspectable but cannot be overwritten implicitly.
    assert(!guarded.Create("Do not overwrite"));
    assert(!guarded.Toggle(0,game2));
    assert(!guarded.Save());
    {
        std::ifstream in(location,std::ios::binary);
        std::string first;
        std::getline(in,first);
        assert(first=="# AuroraX collections v1");
    }

    const std::string largePath=(root/"large.txt").string();
    CollectionStore large(largePath);
    assert(large.Create("Large"));
    size_t added=0;
    for(size_t i=0;i<512;++i) {
        const std::string path=std::string(2000,'X')+std::to_string(i);
        if(!large.Toggle(0,path))break;
        ++added;
    }
    assert(added>100 && added<512);
    assert(large.Save());
    assert(fs::file_size(largePath)<=256*1024);
    CollectionStore largeReload(largePath);
    assert(largeReload.Load());
    assert(largeReload.Items()[0].games.size()==added);
    assert(largeReload.Toggle(0,largeReload.Items()[0].games[0]));
    assert(largeReload.Toggle(0,"another"));
    assert(largeReload.Save());

    // A single oversized line must fail before allocating arbitrary memory.
    const std::string corrupt=(root/"oversize.txt").string();
    {std::ofstream out(corrupt,std::ios::binary);out<<std::string(300*1024,'X');}
    CollectionStore oversized(corrupt);
    assert(!oversized.Load());
    assert(!oversized.Save());
    assert(fs::file_size(corrupt)==300*1024);

    fs::remove_all(root);
}
