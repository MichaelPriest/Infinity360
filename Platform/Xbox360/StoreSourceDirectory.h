#pragma once
#include "StoreCatalog.h"
#include <string>
#include <vector>
namespace StoreSourceDirectory {
struct Reference {const char* id;const char* label;const char* note;};
inline const std::vector<Reference>& References(){
    static const std::vector<Reference> rows={
        {"series360","Series360","Catalogo autorizado TSV necessario"},
        {"xstore","Freestyle Rev800 / X-Store","Catalogo autorizado TSV necessario"},
        {"andromeda","Nexus Andromeda","Metadados locais em Andromeda; TSV para downloads autorizados"},
    };
    return rows;
}
struct Row {std::string id,name,note;int provider=-1;bool filter=true;};
inline std::vector<Row> Rows(const std::vector<StoreProvider>& providers){
    std::vector<Row> rows={{"*","Todos os catalogos","Itens locais e caches aprovados",-1,true},
                           {"","Catalogo local / Homebrew","Itens incluidos e importados localmente",-1,true},
                           {"@unity","XboxUnity","Capas e TU nos detalhes de jogos",-1,false}};
    for(const auto& ref:References()){
        Row row{ref.id,ref.label,ref.note,-1,true};
        for(size_t i=0;i<providers.size();++i)if(providers[i].id==ref.id){
            row.provider=static_cast<int>(i);
            row.note=providers[i].cached?std::to_string(providers[i].items)+" itens em cache":"Configurado; sincronize";
            break;
        }
        rows.push_back(row);
    }
    for(size_t i=0;i<providers.size();++i){
        const auto& p=providers[i];bool known=false;
        for(const auto& ref:References())if(p.id==ref.id){known=true;break;}
        if(!known)rows.push_back({p.id,p.name,p.cached?std::to_string(p.items)+" itens em cache":"Configurado; sincronize",static_cast<int>(i),true});
    }
    return rows;
}
inline bool Matches(const std::string& source,const StoreItem& item){return source=="*"||source==item.provider;}
}
