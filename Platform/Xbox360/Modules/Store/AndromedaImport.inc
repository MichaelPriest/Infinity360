// Included inside StoreCatalog.cpp's private namespace. Uses its bounded I/O and validation helpers.
namespace AndromedaImport {
// Local Nexus format: metadata only, never imports download URLs, hashes or magnets.
inline void Load(const std::string& root,const std::string& storage,std::vector<StoreItem>& items){
    const char* names[]={"Xbox360Games.txt","GodGames.txt","ClassicGames.txt","Xbox360BR.txt","Games.txt"};
    std::set<std::string> ids;for(const auto& item:items)ids.insert(item.id);
    for(const char* name:names){
        std::string data,line;
        if(!readBound(root+"Andromeda/"+name,2*1024*1024,data)||data.find('\0')!=std::string::npos)continue;
        std::istringstream in(data);if(!std::getline(in,line))continue;
        if(!line.empty()&&line.back()=='\r')line.pop_back();
        if(line.compare(0,3,"\xef\xbb\xbf")==0)line.erase(0,3);
        const std::string format=line;
        if(format!="NEXUS_CATALOG_V3"&&format!="NEXUS_GOD_V1"&&format!="NEXUS_CLASSIC_ISO_V1"&&format!="NEXUS_GAMES_V1")continue;
        while(items.size()<12000&&std::getline(in,line)){
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            if(line.empty()||line.size()>4096)continue;
            std::vector<std::string> f;size_t start=0;
            for(size_t i=0;i<=line.size();++i)if(i==line.size()||line[i]=='|'){f.push_back(line.substr(start,i-start));start=i+1;}
            const size_t k=format=="NEXUS_CATALOG_V3"?2:format=="NEXUS_GOD_V1"?1:0;
            const size_t expected=format=="NEXUS_CATALOG_V3"?4:format=="NEXUS_GOD_V1"?3:format=="NEXUS_CLASSIC_ISO_V1"?6:5;
            if(f.size()!=expected||!display(f[k+1],192))continue;
            const bool classic=format=="NEXUS_CLASSIC_ISO_V1";unsigned title=0;
            if(classic){if(f[k].size()!=16||f[k].find_first_not_of("0123456789ABCDEFabcdef")!=std::string::npos)continue;}
            else if(!hex8(f[k],title))continue;
            std::string id="nexus-"+f[k];for(char& ch:id)if(ch>='A'&&ch<='Z')ch=char(ch-'A'+'a');
            if(!ids.insert(id).second)continue;
            StoreItem item{};item.id=id;item.category="game";item.name=f[k+1];item.version=classic?"Xbox classico":"Xbox 360";item.titleId=title;item.provider="andromeda";item.storage=storage;item.rights="Metadados locais; sem pacote autorizado";items.push_back(item);
        }
    }
}
}
