#pragma once
#include "SavedFile.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <algorithm>

// Bounded dashboard-local operational log. No URLs, paths or account data
// are added by callers. Legacy rows are imported as already read.
class NotificationCenter {
public:
    enum Filter { All, Unread, Attention, FilterCount };
    enum Level { Info, Success, Warning };
    struct Item {
        std::string text;
        bool read=false;
        Level level=Info;
        std::string area;
    };
    static constexpr size_t Limit=64;
    static bool Clean(const std::string& text){
        if(text.empty()||text.size()>240)return false;
        for(unsigned char c:text)if(c<32||c==127)return false;
        return true;
    }
    static Item Make(const std::string& text,bool read){
        Item item;item.text=text;item.read=read;
        const auto p=text.find(" | ");
        const auto message=p==std::string::npos?text:text.substr(p+3);
        const auto has=[&](const char* key){return message.find(key)!=std::string::npos;};
        if(has("Falha")||has("falhou")||has("recusad")||has("interrompid")||has("cancelad"))item.level=Warning;
        else if(has("verificad")||has("instalad")||has("concluido")||has("importad")||has("sincronizado")||has("salvo")||has("copiado")||has("recuperada"))item.level=Success;
        item.area=has("FTP")?"Rede":has("lancamento")?"Jogos":
            has("Store")||has("loja")||has("Download")||has("Catalogo")?"Loja":
            has("Backup")||has("Recuperacao")||has("recuperada")||has("TU ")||has("XboxUnity")||has("conteudo")?"Conteudo":
            has("DVD")||has("GOD")?"Discos":
            has("arte")||has("capas")?"Arte":
            has("Copia")||has("copia")||has("pasta")||has("Fila")?"Arquivos":"Sistema";
        return item;
    }
    bool Load(const std::string& path){
        items_.clear();writable_=true;
        std::ifstream in(savedFile(path).c_str(),std::ios::binary);
        if(!in){
            if(fileExists(path)||fileExists(path+".bak"))writable_=false;
            return writable_;
        }
        std::vector<char> raw(32769);
        in.read(raw.data(),static_cast<std::streamsize>(raw.size()));
        if(in.bad()||in.gcount()>32768){writable_=false;return false;}
        std::istringstream lines(std::string(raw.data(),static_cast<size_t>(in.gcount())));
        std::string line;
        while(std::getline(lines,line)){
            if(!line.empty()&&line.back()=='\r')line.pop_back();
            bool read=true;
            if(line.compare(0,5,"N1 U ")==0||line.compare(0,5,"N1 R ")==0){read=line[3]=='R';line.erase(0,5);}
            else if(line.compare(0,3,"N1 ")==0){writable_=false;items_.clear();return false;}
            if(line.empty())continue;
            if(!Clean(line)){writable_=false;items_.clear();return false;}
            Append(line,read);
        }
        return true;
    }
    bool Save(const std::string& path)const{
        if(!writable_)return false;
        std::ofstream out((path+".tmp").c_str(),std::ios::binary|std::ios::trunc);
        for(const auto& item:items_)out<<(item.read?"N1 R ":"N1 U ")<<item.text<<"\n";
        out.close();return bool(out)&&replaceSavedFile(path);
    }
    bool Append(const std::string& text,bool read=false){
        if(!Clean(text))return false;
        items_.push_back(Make(text,read));
        if(items_.size()>Limit)items_.erase(items_.begin());
        return true;
    }
    size_t UnreadCount()const{
        size_t n=0;for(const auto& item:items_)if(!item.read)++n;return n;
    }
    std::vector<size_t> Visible(Filter filter)const{
        std::vector<size_t> result;
        for(size_t i=items_.size();i>0;--i){
            const auto& item=items_[i-1];
            if(filter==All||(filter==Unread&&!item.read)||(filter==Attention&&item.level==Warning))result.push_back(i-1);
        }
        return result;
    }
    const Item& At(size_t i)const{return items_[i];}
    bool Mark(size_t i){if(i>=items_.size()||items_[i].read)return false;items_[i].read=true;return true;}
    void MarkAll(){for(auto& item:items_)item.read=true;}
    bool Writable()const{return writable_;}
private:
    std::vector<Item> items_;
    bool writable_=true;
};
