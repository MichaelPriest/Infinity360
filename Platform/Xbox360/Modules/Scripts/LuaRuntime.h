#pragma once
#include <string>
#include <string_view>
#include <fstream>
#include <cstdlib>
#include <cstddef>
#include <cstdio>
#include <vector>
#ifdef INFINITY_WITH_LUA
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#endif
namespace InfinityLua {
// Borrowed synchronous view: no executable paths or writable console handles.
struct GameView{const std::string* name=nullptr;const std::string* device=nullptr;bool favorite=false,package=false,hidden=false,pinned=false;std::string_view storage="Local";};
inline std::string_view StorageLabel(std::string_view path){
    const auto end=path.find_first_of(":/\\");
    if(end==path.npos||end==0||end>32||path[end]!=':')return "Local";
    return path.substr(0,end+1);
}
struct MetadataView{
    bool known=false;
    unsigned titleId=0,mediaId=0,version=0,baseVersion=0,discNumber=0,discCount=0,contentType=0,volumeType=0;
};
struct Library{size_t count=0;const void* context=nullptr;GameView (*get)(const void*,size_t)=nullptr;MetadataView (*metadata)(const void*,size_t)=nullptr;};
struct FilterResult{bool ok=false;std::vector<size_t> indices;std::string message;};
struct SubtitleResult{
    std::vector<std::string> lines;
    std::string error;
};
constexpr size_t MaxSourceBytes=65536;
constexpr size_t MaxSubtitleBytes=96;
constexpr size_t MaxSubtitleRows=12;
inline bool Available(){
#ifdef INFINITY_WITH_LUA
    return true;
#else
    return false;
#endif
}
#ifdef INFINITY_WITH_LUA
struct Budget{
    size_t used=0;unsigned ticks=0;Library library{};
    static constexpr size_t MaxMetadata=128;
    struct Cached{size_t index=0;MetadataView data{};};
    Cached cache[MaxMetadata]{};size_t cached=0;
};
inline void* Alloc(void* ud,void* ptr,size_t old,size_t bytes){
    auto& b=*static_cast<Budget*>(ud);
    if(!ptr)old=0;
    if(!bytes){std::free(ptr);b.used-=old;return nullptr;}
    if(bytes>old&&bytes-old>2*1024*1024-b.used)return nullptr;
    void* next=std::realloc(ptr,bytes);
    if(next)b.used=b.used-old+bytes;
    return next;
}
inline void Limit(lua_State* L,lua_Debug*){
    void* ud=nullptr;lua_getallocf(L,&ud);
    if(++static_cast<Budget*>(ud)->ticks>=100)luaL_error(L,"limite de instrucoes");
}
inline void Field(lua_State* L,const char* key,const std::string* value){
    if(value)lua_pushlstring(L,value->data(),value->size()>256?256:value->size());else lua_pushliteral(L,"");
    lua_setfield(L,-2,key);
}
inline int GameAt(lua_State* L){
    void* ud=nullptr;lua_getallocf(L,&ud);
    const auto& library=static_cast<Budget*>(ud)->library;
    const auto index=luaL_checkinteger(L,1);
    if(index<=0||static_cast<unsigned long long>(index)>library.count||!library.get){lua_pushnil(L);return 1;}
    const GameView g=library.get(library.context,static_cast<size_t>(index-1));
    lua_createtable(L,0,8);
    Field(L,"name",g.name);Field(L,"device",g.device);
    lua_pushlstring(L,g.storage.data(),g.storage.size());lua_setfield(L,-2,"storage");
    lua_pushboolean(L,g.favorite);lua_setfield(L,-2,"favorite");
    lua_pushboolean(L,g.package);lua_setfield(L,-2,"package");
    lua_pushboolean(L,g.hidden);lua_setfield(L,-2,"hidden");
    lua_pushboolean(L,g.pinned);lua_setfield(L,-2,"pinned");
    lua_pushinteger(L,index);lua_setfield(L,-2,"index");
    return 1;
}
inline void UIntField(lua_State* L,const char* key,unsigned value){
    lua_pushinteger(L,static_cast<lua_Integer>(value));lua_setfield(L,-2,key);
}
inline int MetadataAt(lua_State* L){
    void* ud=nullptr;lua_getallocf(L,&ud);
    auto& b=*static_cast<Budget*>(ud);
    const auto i=luaL_checkinteger(L,1);
    if(i<=0||static_cast<unsigned long long>(i)>b.library.count||!b.library.metadata){
        lua_pushnil(L);lua_pushliteral(L,"indisponivel");return 2;
    }
    const auto index=static_cast<size_t>(i-1);
    size_t at=0;while(at<b.cached&&b.cache[at].index!=index)++at;
    if(at==b.cached){
        if(b.cached>=Budget::MaxMetadata){lua_pushnil(L);lua_pushliteral(L,"limite de metadados");return 2;}
        b.cache[at].index=index;b.cache[at].data=b.library.metadata(b.library.context,index);++b.cached;
    }
    const auto& m=b.cache[at].data;
    if(!m.known){lua_pushnil(L);lua_pushliteral(L,"cabecalho indisponivel");return 2;}
    lua_createtable(L,0,10);
    UIntField(L,"title_id",m.titleId);UIntField(L,"media_id",m.mediaId);
    UIntField(L,"version",m.version);UIntField(L,"base_version",m.baseVersion);
    UIntField(L,"disc_number",m.discNumber);UIntField(L,"disc_count",m.discCount);
    UIntField(L,"content_type",m.contentType);UIntField(L,"volume_type",m.volumeType);
    char text[32];
    std::snprintf(text,sizeof text,"%08X",m.titleId);
    lua_pushstring(L,text);lua_setfield(L,-2,"title_id_hex");
    std::snprintf(text,sizeof text,"%u.%u.%u.%u",m.version>>28,(m.version>>24)&15u,(m.version>>8)&65535u,m.version&255u);
    lua_pushstring(L,text);lua_setfield(L,-2,"version_text");
    return 1;
}
inline int Setup(lua_State* L){
    void* ud=nullptr;lua_getallocf(L,&ud);
    const auto count=static_cast<Budget*>(ud)->library.count;
    luaL_requiref(L,"_G",luaopen_base,1);lua_pop(L,1);
    luaL_requiref(L,"table",luaopen_table,1);lua_pop(L,1);
    const char* blocked[]={"dofile","loadfile","load","collectgarbage","pcall","xpcall","setmetatable","getmetatable","print","warn"};
    for(const auto* name:blocked){lua_pushnil(L);lua_setglobal(L,name);}
    lua_newtable(L);lua_pushinteger(L,static_cast<lua_Integer>(count));lua_setfield(L,-2,"game_count");
    lua_pushcfunction(L,GameAt);lua_setfield(L,-2,"game");
    lua_pushcfunction(L,MetadataAt);lua_setfield(L,-2,"metadata");
    lua_pushinteger(L,3);lua_setfield(L,-2,"api_version");
    lua_setglobal(L,"infinity");return 0;
}
inline std::string Result(lua_State* L,const char* fallback){
    if(lua_type(L,-1)==LUA_TNUMBER)return lua_isinteger(L,-1)?std::to_string(lua_tointeger(L,-1)):std::to_string(lua_tonumber(L,-1));
    if(lua_type(L,-1)!=LUA_TSTRING)return fallback;
    size_t n=0;const char* s=lua_tolstring(L,-1,&n);if(!s)return fallback;
    std::string msg(s,n>180?180:n);for(auto& ch:msg)if(static_cast<unsigned char>(ch)<32)ch=' ';
    return msg;
}
inline int ExecuteChunk(lua_State* L,const std::string& code,const char* name){
    lua_pushcfunction(L,Setup);
    int rc=lua_pcall(L,0,0,0);
    if(rc==LUA_OK)rc=luaL_loadbufferx(L,code.data(),code.size(),name,"t");
    if(rc==LUA_OK){
        lua_sethook(L,Limit,LUA_MASKCOUNT,1000);
        rc=lua_pcall(L,0,1,0);
    }
    return rc;
}
#endif
inline std::string RunText(const std::string& code,const Library& library){
    if(code.size()>MaxSourceBytes)return "Script excede 64 KiB.";
#ifdef INFINITY_WITH_LUA
    Budget b;b.library=library;lua_State* L=lua_newstate(Alloc,&b);if(!L)return "Memoria Lua indisponivel.";
    const int rc=ExecuteChunk(L,code,"script");
    auto result=rc==LUA_OK?std::string("Lua: ")+Result(L,"concluido."):std::string("Lua erro: ")+Result(L,"execucao recusada.");
    lua_close(L);return result;
#else
    (void)library;return "Runtime Lua nao incluido nesta build.";
#endif
}
inline std::string RunText(const std::string& code,size_t count){return RunText(code,Library{count});}
inline FilterResult FilterText(const std::string& code,const Library& library,bool ordered=false){
    FilterResult result;
    if(code.size()>MaxSourceBytes||library.count>32768){result.message="Limite de filtro excedido.";return result;}
#ifdef INFINITY_WITH_LUA
    Budget b;b.library=library;lua_State* L=lua_newstate(Alloc,&b);
    if(!L){result.message="Memoria Lua indisponivel.";return result;}
    const int rc=ExecuteChunk(L,code,"filter");
    if(rc!=LUA_OK)result.message="Lua erro: "+Result(L,"filtro recusado.");
    else if(lua_type(L,-1)!=LUA_TTABLE)result.message="Filtro deve retornar uma lista de indices.";
    else{
        const auto n=lua_rawlen(L,-1);bool valid=n<=library.count&&(!ordered||n==library.count);
        std::vector<unsigned char> seen(library.count,0);
        std::vector<size_t> order;if(valid&&ordered)order.resize(n);
        size_t entries=0;
        if(valid){
            lua_pushnil(L);
            while(lua_next(L,-2)){
                if(!lua_isinteger(L,-2)||!lua_isinteger(L,-1)){valid=false;lua_pop(L,2);break;}
                const auto key=lua_tointeger(L,-2),value=lua_tointeger(L,-1);
                if(key<1||static_cast<unsigned long long>(key)>n||value<1||static_cast<unsigned long long>(value)>library.count||seen[static_cast<size_t>(value-1)]){
                    valid=false;lua_pop(L,2);break;
                }
                seen[static_cast<size_t>(value-1)]=1;
                if(ordered)order[static_cast<size_t>(key-1)]=static_cast<size_t>(value-1);
                ++entries;lua_pop(L,1);
            }
        }
        if(valid&&entries==n){
            if(ordered)result.indices.swap(order);
            else for(size_t i=0;i<seen.size();++i)if(seen[i])result.indices.push_back(i);
            result.ok=true;result.message=std::string(ordered?"Ordenacao aplicada: ":"Filtro aplicado: ")+std::to_string(result.indices.size())+" jogos.";
        }else result.message=ordered?"Ordenacao invalida: inclua todos os indices uma vez.":"Filtro invalido: indices unicos, inteiros e lista sem lacunas.";
    }
    lua_close(L);
#else
    (void)library;result.message="Runtime Lua nao incluido nesta build.";
#endif
    return result;
}
// A formatter shares one VM and instruction/metadata budget across a visible page.
// The returned strings own their bytes after the VM is closed.
inline SubtitleResult SubtitlePage(const std::string& code,const Library& library,const std::vector<size_t>& indices){
    if(code.size()>MaxSourceBytes)return {{},"Script excede 64 KiB."};
    if(indices.size()>MaxSubtitleRows)return {{},"Pagina de legendas excedida."};
#ifdef INFINITY_WITH_LUA
    Budget budget;
    budget.library=library;
    lua_State* L=lua_newstate(Alloc,&budget);
    if(!L)return {{},"Memoria Lua indisponivel."};
    SubtitleResult result;
    int rc=ExecuteChunk(L,code,"subtitle");
    if(rc==LUA_OK && !lua_isfunction(L,-1))result.error="A legenda deve retornar uma funcao.";
    for(size_t index:indices){
        if(rc!=LUA_OK || !result.error.empty())break;
        if(index>=library.count){result.error="Indice de jogo invalido.";break;}
        lua_pushvalue(L,-1);
        lua_pushinteger(L,static_cast<lua_Integer>(index+1));
        rc=lua_pcall(L,1,1,0);
        if(rc!=LUA_OK)break;
        if(lua_isnil(L,-1)){
            result.lines.emplace_back();
        }else if(lua_type(L,-1)==LUA_TSTRING){
            size_t size=0;
            const char* bytes=lua_tolstring(L,-1,&size);
            if(size>MaxSubtitleBytes){result.error="Legenda excede 96 bytes.";break;}
            const std::string_view line(bytes,size);
            for(unsigned char ch:line){
                if(ch<32 || ch==127){result.error="A legenda deve ter uma unica linha de texto.";break;}
            }
            if(!result.error.empty())break;
            result.lines.emplace_back(line);
        }else{
            result.error="A legenda deve retornar texto ou nil.";
            break;
        }
        lua_pop(L,1);
    }
    if(rc!=LUA_OK)result.error="Lua erro: "+Result(L,"legenda recusada.");
    lua_close(L);
    if(!result.error.empty())result.lines.clear();
    return result;
#else
    (void)library;
    return {{},"Runtime Lua nao incluido nesta build."};
#endif
}
inline std::string ReadSource(const std::string& path,std::string& code){
    std::ifstream f(path.c_str(),std::ios::binary|std::ios::ate);
    const std::streamoff length=f.tellg();
    if(length<0 || length>static_cast<std::streamoff>(MaxSourceBytes))return "Script ausente ou acima de 64 KiB.";
    code.resize(static_cast<size_t>(length));
    f.seekg(0);
    if(length && !f.read(&code[0],length))return "Falha de leitura do script.";
    return {};
}
inline FilterResult FilterFile(const std::string& path,const Library& library,bool ordered=false){
    std::string code;
    const auto error=ReadSource(path,code);
    if(!error.empty())return {false,{},error};
    return FilterText(code,library,ordered);
}
inline std::string RunFile(const std::string& path,const Library& library){
    std::string code;
    const auto error=ReadSource(path,code);
    return error.empty()?RunText(code,library):error;
}
}
