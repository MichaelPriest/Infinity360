#pragma once

#include <string>
#include <vector>

// AuroraX extensions v1 are locally installed XEX applications.
// This registry NEVER loads executable modules into dashboard memory.
struct PluginDescriptor {
    std::string id;
    std::string name;
    std::string version;
    std::string category; // "app" or "emulator"
    std::string executable;
    bool manifestEnabled=true,headerValid=false,conflict=false;
};

class PluginRegistry {
public:
    explicit PluginRegistry(const std::vector<std::string>& roots);
    static std::vector<std::string> DefaultRoots(const std::string& appDirectory);
    std::vector<PluginDescriptor> Scan() const;
    std::vector<PluginDescriptor> Inventory() const;
    static std::string Key(const std::string& id);

private:
    std::vector<std::string> roots_;
};
