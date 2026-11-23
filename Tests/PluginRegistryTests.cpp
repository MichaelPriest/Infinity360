#include "PluginRegistry.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>

namespace fs=std::filesystem;

void install(const fs::path& root,const std::string& directory,
             const std::string& id,const std::string& category,
             bool enabled=true,const std::string& entry="default.xex") {
    const fs::path folder=root/directory;
    fs::create_directories(folder);
    std::ofstream(folder/"plugin.axplugin")
        << "schema=1\n"
        << "id=" << id << "\n"
        << "name=" << directory << "\n"
        << "version=0.1.0\n"
        << "category=" << category << "\n"
        << "entry=" << entry << "\n"
        << "enabled=" << (enabled?"true":"false") << "\n";
    std::ofstream(folder/"default.xex")<<"fixture - not executable on console";
}

int main() {
    const fs::path root=fs::temp_directory_path()/
        ("aurorax-plugins-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    const fs::path p1=root/"Plugins",p2=root/"Other";
    fs::create_directories(p1);
    fs::create_directories(p2);
    install(p1,"Tools","aurorax.tools","app");
    install(p1,"Emu","aurorax.emu","emulator");
    install(p1,"Disabled","aurorax.disabled","app",false);
    install(p1,"Traversal","aurorax.badpath","app",true,"../default.xex");
    install(p1,"NoBinary","aurorax.missing","app",true,"missing.xex");
    install(p2,"Duplicate","aurorax.tools","app");
    PluginRegistry registry({p1.string(),p2.string()});
    const auto plugins=registry.Scan();
    assert(plugins.size()==2);
    assert(plugins[0].id=="aurorax.emu");
    assert(plugins[0].category=="emulator");
    assert(plugins[1].id=="aurorax.tools");
    assert(plugins[1].version=="0.1.0");
    assert(plugins[1].executable==(p1/"Tools"/"default.xex").string());

    // Unsafe identifiers and unknown configuration keys must not be accepted.
    install(p1,"Unsafe","a/../../escape","app");
    install(p1,"UnknownKey","aurorax.unknown","app");
    {
        std::ofstream file(p1/"UnknownKey"/"plugin.axplugin",std::ios::app);
        file<<"remote_url=https://example.invalid/evil.xex\n";
    }
    assert(registry.Scan().size()==2);

#ifndef _WIN32
    // Filesystem links cannot evade the allowed local plugin directories.
    std::error_code error;
    fs::create_directory_symlink(p1/"Tools",p1/"Linked",error);
    assert(registry.Scan().size()==2);
#endif

    fs::remove_all(root);
}
