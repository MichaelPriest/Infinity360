#include "PluginRegistry.h"
#include "GameScanner.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>

#ifndef _XBOX
#include <filesystem>
#endif

namespace {
const size_t MaxPlugins=64;
const size_t MaxManifestBytes=4096;

std::string join(const std::string& base,const std::string& name) {
    if(base.empty()) return name;
    const char last=base[base.size()-1];
    if(last=='/' || last=='\\') return base+name;
    return base+(base.find('\\')!=std::string::npos ? "\\" : "/")+name;
}
std::string trim(const std::string& input) {
    const size_t start=input.find_first_not_of(" \t\r");
    if(start==std::string::npos) return "";
    const size_t end=input.find_last_not_of(" \t\r");
    return input.substr(start,end-start+1);
}
bool safeFilename(const std::string& name) {
    if(name.empty() || name.size()>64 || name=="." || name=="..") return false;
    for(size_t i=0;i<name.size();++i) {
        const unsigned char ch=(unsigned char)name[i];
        if(!(std::isalnum(ch) || ch=='.' || ch=='_' || ch=='-')) return false;
    }
    return true;
}
bool safeId(const std::string& id) {
    return safeFilename(id) && id.find("..")==std::string::npos;
}
bool displayName(const std::string& name) {
    if(name.empty() || name.size()>80) return false;
    for(size_t i=0;i<name.size();++i)
        if((unsigned char)name[i]<32u || (unsigned char)name[i]==127u) return false;
    return true;
}
bool isXex(const std::string& entry) {
    if(!safeFilename(entry) || entry.size()<5) return false;
    std::string ext=entry.substr(entry.size()-4);
    for(size_t i=0;i<ext.size();++i)
        ext[i]=(char)std::tolower((unsigned char)ext[i]);
    return ext==".xex";
}
bool normalFile(const std::string& path) {
#ifndef _XBOX
    std::error_code ec;
    if(std::filesystem::is_symlink(std::filesystem::symlink_status(path,ec))) return false;
#endif
    std::ifstream file(path.c_str(),std::ios::binary);
    return file.good();
}
bool normalFolder(const std::string& path) {
#ifndef _XBOX
    std::error_code ec;
    if(std::filesystem::is_symlink(std::filesystem::symlink_status(path,ec))) return false;
#endif
    return true;
}
bool readManifest(const std::string& path, PluginDescriptor& plugin,
                  std::string& entry, bool& enabled) {
    std::ifstream file(path.c_str(),std::ios::binary);
    if(!file) return false;
    plugin=PluginDescriptor();
    enabled=true;
    bool schemaFound=false;
    size_t bytes=0;
    std::string line;
    while(std::getline(file,line)) {
        bytes+=line.size()+1;
        if(bytes>MaxManifestBytes || line.size()>256) return false;
        line=trim(line);
        if(line.empty() || line[0]=='#') continue;
        const size_t equal=line.find('=');
        if(equal==std::string::npos) return false;
        const std::string key=trim(line.substr(0,equal));
        const std::string value=trim(line.substr(equal+1));
        if(key=="schema") { if(value!="1") return false; schemaFound=true; }
        else if(key=="id") plugin.id=value;
        else if(key=="name") plugin.name=value;
        else if(key=="version") plugin.version=value;
        else if(key=="category") plugin.category=value;
        else if(key=="entry") entry=value;
        else if(key=="enabled") {
            if(value=="true") enabled=true;
            else if(value=="false") enabled=false;
            else return false;
        } else return false;
    }
    return !file.bad() && schemaFound && safeId(plugin.id) && displayName(plugin.name) &&
        plugin.version.size()<=24 &&
        (plugin.category=="app" || plugin.category=="emulator") &&
        isXex(entry);
}
}

PluginRegistry::PluginRegistry(const std::vector<std::string>& roots):roots_(roots) {}

std::vector<std::string> PluginRegistry::DefaultRoots(const std::string& appDirectory) {
    std::vector<std::string> roots;
    roots.push_back(join(appDirectory,"Plugins"));
#ifdef _XBOX
    roots.push_back("HDD1:\\Infinity360\\Plugins");
    roots.push_back("USB0:\\Infinity360\\Plugins");
    roots.push_back("USB1:\\Infinity360\\Plugins");
    // Legacy locations remain readable.
    roots.push_back("HDD1:\\AuroraX\\Plugins");
    roots.push_back("USB0:\\AuroraX\\Plugins");
    roots.push_back("USB1:\\AuroraX\\Plugins");
#elif defined(__XSDK__)
    const char* drives[]={"hdd:/","usb0:/","usb1:/","usb2:/"};
    for(const char* drive:drives){roots.push_back(std::string(drive)+"Infinity360/Plugins");roots.push_back(std::string(drive)+"AuroraX/Plugins");}
#endif
    return roots;
}

std::vector<PluginDescriptor> PluginRegistry::Scan() const {
    std::vector<PluginDescriptor> found;
    std::set<std::string> seen;
    for(size_t root=0;root<roots_.size() && found.size()<MaxPlugins;++root) {
        if(!normalFolder(roots_[root])) continue;
        std::vector<FolderEntry> folders;
        if(!ReadFolder(roots_[root],folders)) continue;
        for(size_t i=0;i<folders.size() && found.size()<MaxPlugins;++i) {
            const FolderEntry& folder=folders[i];
            if(!folder.directory || !safeFilename(folder.name)) continue;
            const std::string directory=join(roots_[root],folder.name);
            if(!normalFolder(directory)) continue;
            const std::string manifest=join(directory,"plugin.axplugin");
            if(!normalFile(manifest)) continue;
            PluginDescriptor plugin;
            std::string entry;
            bool enabled=true;
            if(!readManifest(manifest,plugin,entry,enabled) || !enabled) continue;
            if(seen.find(plugin.id)!=seen.end()) continue;
            plugin.executable=join(directory,entry);
            if(!normalFile(plugin.executable)) continue;
            seen.insert(plugin.id);
            found.push_back(plugin);
        }
    }
    return found;
}
