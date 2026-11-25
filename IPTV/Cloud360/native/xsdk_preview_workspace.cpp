// Conservative read-only explorer adapter for the XSDK menu preview.
// No writes, copies, moves, deletes or hard-link emulation on unverified FATX.
#include "FileWorkspace.h"
#include <filesystem>
#include <cctype>
namespace fs=std::filesystem;
namespace {
std::string trim(std::string p){while(p.size()>1&&(p.back()=='/'||p.back()=='\\'))p.pop_back();return p;}
}
FileWorkspace::FileWorkspace(const std::string& appDirectory){roots_.push_back(appDirectory);}
bool FileWorkspace::ValidName(const std::string& name){
    if(name.empty()||name.size()>42||name=="."||name==".."||name.front()==' '||name.back()==' '||name.back()=='.')return false;
    for(unsigned char ch:name)if(!(std::isalnum(ch)||ch==' '||ch=='.'||ch=='-'||ch=='_'))return false;
    return true;
}
std::string FileWorkspace::Join(const std::string& parent,const std::string& name){
    return parent.empty()?name:parent+(parent.back()=='/'||parent.back()=='\\'?"":"/")+name;
}
bool FileWorkspace::Allowed(const std::string& path,const std::vector<std::string>& roots)const{
    if(path.empty()||path.size()>240||path.find('\\')!=std::string::npos)return false;
    for(const auto& part:fs::path(path))if(part=="."||part=="..")return false;
    for(const auto& root:roots){const std::string r=trim(root);
        if(trim(path)==r||path.compare(0,r.size()+1,r+"/")==0)return true;}
    return false;
}
bool FileWorkspace::CanBrowse(const std::string& path)const{
    if(!Allowed(path,roots_))return false;
    std::error_code ec;return fs::is_directory(path,ec)&&!ec;
}
bool FileWorkspace::CanWrite(const std::string&)const{return false;}
bool FileWorkspace::CanReadPersonalFile(const std::string&)const{return false;}
bool FileWorkspace::Create(const std::string&,const std::string&)const{return false;}
bool FileWorkspace::Rename(const std::string&,const std::string&,const std::string&)const{return false;}
bool FileWorkspace::Remove(const std::string&,const std::string&)const{return false;}
bool FileWorkspace::Move(const std::string&,const std::string&,const std::string&)const{return false;}
bool FileWorkspace::Copy(const std::string&,const std::string&,const std::string&)const{return false;}
bool FileWorkspace::PrepareCopy(const std::string&,const std::string&,const std::string&,std::string& from,std::string& to)const{from.clear();to.clear();return false;}
std::string FileWorkspace::WorkspaceFor(const std::string&)const{return "";}
bool FileWorkspace::PlanCopies(const std::string&,const std::vector<std::string>&,const std::string&,std::vector<WorkspaceCopyStep>& out)const{out.clear();return false;}
FileTransfer::FileTransfer():state_(Idle),done_(0),total_(0),output_(-1){}
FileTransfer::~FileTransfer(){}
bool FileTransfer::Start(const FileWorkspace&,const std::string&,const std::string&,const std::string&){
    state_=Failed;error_="Copia indisponivel na previa XSDK.";return false;
}
void FileTransfer::Tick(){}
void FileTransfer::Cancel(){}
unsigned FileTransfer::Percent()const{return 0;}
