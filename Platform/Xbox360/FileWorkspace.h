#pragma once
#include <string>
#include <vector>
#include <fstream>

// Local explorer: browse permitted mounts; write only to user workspaces.
class FileWorkspace {
public:
    explicit FileWorkspace(const std::string& appDirectory);
    const std::vector<std::string>& Roots() const { return roots_; }
    static bool ValidName(const std::string& name);
    static std::string Join(const std::string& parent,const std::string& name);
    bool CanBrowse(const std::string& path) const;
    bool CanWrite(const std::string& directory) const;
    bool CanReadPersonalFile(const std::string& path) const;
    bool Create(const std::string& directory,const std::string& name) const;
    bool Rename(const std::string& directory,const std::string& oldName,
                const std::string& newName) const;
    bool Copy(const std::string& sourceDir,const std::string& name,
              const std::string& destDir) const;
    // Validate a source file and an unused destination within personal storage.
    bool PrepareCopy(const std::string& sourceDir,const std::string& name,
                     const std::string& destDir,std::string& from,std::string& to) const;
    std::string WorkspaceFor(const std::string& root) const;
private:
    std::vector<std::string> roots_, workspaces_;
    bool Allowed(const std::string& path,const std::vector<std::string>& roots) const;
};

class FileTransfer {
public:
    enum State { Idle, Running, Completed, Cancelled, Failed };
    FileTransfer();
    ~FileTransfer();
    bool Start(const FileWorkspace& workspace,const std::string& sourceDir,
               const std::string& name,const std::string& destDir);
    // Process one bounded chunk. Call from the dashboard's frame loop.
    void Tick();
    void Cancel();
    State Status() const { return state_; }
    unsigned Percent() const;
    unsigned long long Done() const { return done_; }
    unsigned long long Total() const { return total_; }
    const std::string& Name() const { return name_; }
    const std::string& Error() const { return error_; }
private:
    FileTransfer(const FileTransfer&);
    FileTransfer& operator=(const FileTransfer&);
    void Cleanup();
    void Fail(const char* reason);
    State state_;
    unsigned long long done_,total_;
    std::string name_,target_,temporary_,error_;
    std::ifstream input_;
    // The output is created exclusively; never truncate a pre-existing file.
#if defined(_XBOX) || defined(_WIN32)
    void* output_;
#else
    int output_;
#endif
};
