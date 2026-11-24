#pragma once
#include <string>
#include <vector>

// Local explorer: browse permitted mounts; write only to user workspaces.
class FileWorkspace {
public:
    explicit FileWorkspace(const std::string& appDirectory);
    const std::vector<std::string>& Roots() const { return roots_; }
    static bool ValidName(const std::string& name);
    static std::string Join(const std::string& parent,const std::string& name);
    bool CanBrowse(const std::string& path) const;
    bool CanWrite(const std::string& directory) const;
    bool Create(const std::string& directory,const std::string& name) const;
    bool Rename(const std::string& directory,const std::string& oldName,
                const std::string& newName) const;
    bool Copy(const std::string& sourceDir,const std::string& name,
              const std::string& destDir) const;
    std::string WorkspaceFor(const std::string& root) const;
private:
    std::vector<std::string> roots_, workspaces_;
    bool Allowed(const std::string& path,const std::vector<std::string>& roots) const;
};
