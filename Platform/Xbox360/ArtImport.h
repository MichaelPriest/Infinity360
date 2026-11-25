#pragma once
#include "FileWorkspace.h"
#include <string>
#include <vector>
namespace ArtImport {
bool Convert(const std::string& encoded,bool cover,std::vector<unsigned char>& rgba);
std::string Folder(const std::string& root,unsigned title);
bool Save(const FileWorkspace& ws,const std::string& root,unsigned title,bool cover,const std::string& encoded,std::string& error);
// Move personal artwork between active and archived slots without overwriting.
bool HasArchived(const FileWorkspace& ws,const std::string& root,unsigned title,bool cover);
bool SetArchived(const FileWorkspace& ws,const std::string& root,unsigned title,bool cover,bool archived,std::string& error);
bool Local(const FileWorkspace& ws,const std::string& root,unsigned title,bool cover,std::string& error);
}
