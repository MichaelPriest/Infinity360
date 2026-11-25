#pragma once
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cctype>

// Diagnostic only. Reads a local DashLaunch INI; never loads modules or
// edits boot, network, identity, authentication or security settings.
struct ResidentPluginSlot {
    std::string path;
    bool duplicate;
    ResidentPluginSlot():duplicate(false){}
};
inline std::string residentTrim(const std::string& text) {
    size_t a=text.find_first_not_of(" \t\r\n"),b=text.find_last_not_of(" \t\r\n");
    return a==std::string::npos?"":text.substr(a,b-a+1);
}
inline std::string residentLower(std::string text) {
    for(size_t i=0;i<text.size();++i)text[i]=(char)std::tolower((unsigned char)text[i]);
    return text;
}
inline bool readResidentPlugins(const std::string& file,std::vector<ResidentPluginSlot>& out) {
    out.assign(5,ResidentPluginSlot());
    std::ifstream in(file.c_str(),std::ios::binary);
    if(!in)return false;
    const size_t limit=65536;
    std::vector<char> bytes(limit+1);
    in.read(&bytes[0],static_cast<std::streamsize>(bytes.size()));
    if(in.bad()||in.gcount()>static_cast<std::streamsize>(limit))return false;
    std::istringstream lines(std::string(&bytes[0],static_cast<size_t>(in.gcount())));
    std::string line;bool section=false,seen[5]={false,false,false,false,false};
    while(std::getline(lines,line)) {
        line=residentTrim(line);
        if(line.empty()||line[0]==';'||line[0]=='#')continue;
        if(line[0]=='[') {section=residentLower(line)=="[plugins]";continue;}
        if(!section)continue;
        size_t equal=line.find('=');
        if(equal==std::string::npos)continue;
        std::string name=residentLower(residentTrim(line.substr(0,equal)));
        if(name.size()!=7||name.compare(0,6,"plugin")||name[6]<'1'||name[6]>'5')continue;
        unsigned slot=name[6]-'1';
        std::string value=residentTrim(line.substr(equal+1));
        if(value.size()>240)return false;
        for(size_t i=0;i<value.size();++i)if((unsigned char)value[i]<32||value[i]==127)return false;
        if(seen[slot])out[slot].duplicate=true;
        seen[slot]=true;out[slot].path=value;
    }
    return true;
}
inline bool isLiveEmulationReference(const std::string& path) {
    size_t sep=path.find_last_of("\\/");
    return residentLower(path.substr(sep==std::string::npos?0:sep+1))=="liveemulation.xex";
}
