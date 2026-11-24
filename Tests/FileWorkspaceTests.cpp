#include "FileWorkspace.h"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs=std::filesystem;
int main() {
    const auto root=fs::temp_directory_path()/("aurorax-files-"+std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root);
    const std::string app=root.string()+"/";
    FileWorkspace files(app);
    assert(files.CanBrowse(app));
    assert(!files.CanWrite(app));
    assert(!files.Create(app,"Games"));
    assert(!files.Create(app,"../escape"));
    assert(files.Create(app,"UserFiles"));
    assert(!files.Create(app,"UserFiles"));
    const std::string user=(root/"UserFiles").string();
    assert(files.CanWrite(user));
    assert(files.Create(user,"Documents"));
    assert(!files.Create(user,".."));
    const std::string docs=(root/"UserFiles"/"Documents").string();
    std::ofstream(root/"test.txt")<<"preserve original";
    assert(files.Copy(app,"test.txt",docs));
    assert(!files.Copy(app,"test.txt",docs));
    assert(!files.Rename(app,"test.txt","other.txt"));
    assert(files.Rename(docs,"test.txt","renamed.txt"));
    assert(fs::exists(root/"test.txt"));
    assert(fs::exists(fs::path(docs)/"renamed.txt"));
    assert(!fs::exists(fs::path(docs)/"test.txt"));
    assert(!files.Rename(docs,"renamed.txt","../test.txt"));
    assert(!files.Copy(app,"test.txt",app));
    assert(!files.Copy(app,"UserFiles",docs));
    {std::ofstream big(root/"big.bin",std::ios::binary);
     big.seekp(8*1024*1024);big.put('x');}
    assert(!files.Copy(app,"big.bin",docs));
#ifndef _WIN32
    std::error_code ec;
    fs::create_directory_symlink(root,fs::path(docs)/"link",ec);
    if(!ec) {
        assert(!files.CanBrowse((fs::path(docs)/"link").string()));
        assert(!files.Create((fs::path(docs)/"link").string(),"unexpected"));
    }
#endif
    fs::remove_all(root);
}
