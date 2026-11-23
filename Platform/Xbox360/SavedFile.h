#pragma once
#include <cstdio>
#include <fstream>
#include <string>

inline bool fileExists(const std::string& path) {
    std::ifstream in(path.c_str(),std::ios::binary);
    return in.good();
}
// FATX cannot rename over an existing file. The backup also covers a power loss
// between renames. Readers use it if the primary file is missing.
inline bool replaceSavedFile(const std::string& path) {
    const std::string tmp=path+".tmp", backup=path+".bak";
    const bool old=fileExists(path);
    if(old) {
        std::remove(backup.c_str());
        if(std::rename(path.c_str(),backup.c_str())) return false;
    }
    if(std::rename(tmp.c_str(),path.c_str())) {
        if(old) std::rename(backup.c_str(),path.c_str());
        return false;
    }
    std::remove(backup.c_str());
    return true;
}
inline std::string savedFile(const std::string& path) { return fileExists(path) ? path : path+".bak"; }
