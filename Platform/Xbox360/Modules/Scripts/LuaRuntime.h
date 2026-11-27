#pragma once
#include <string>
#include <fstream>
#include <cstdlib>
#include <cstddef>
#ifdef INFINITY_WITH_LUA
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}
#endif
namespace InfinityLua {
inline bool Available(){
#ifdef INFINITY_WITH_LUA
    return true;
#else
    return false;
#endif
}
#ifdef INFINITY_WITH_LUA
struct Budget{size_t used=0;unsigned ticks=0;};
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
inline int Setup(lua_State* L){
    const auto count=lua_tointeger(L,1);
    luaL_requiref(L,"_G",luaopen_base,1);lua_pop(L,1);
    const char* blocked[]={"dofile","loadfile","load","collectgarbage","pcall","xpcall","setmetatable","getmetatable","print","warn"};
    for(const auto* name:blocked){lua_pushnil(L);lua_setglobal(L,name);}
    lua_newtable(L);lua_pushinteger(L,count);lua_setfield(L,-2,"game_count");
    lua_setglobal(L,"infinity");return 0;
}
inline std::string Result(lua_State* L,const char* fallback){
    if(lua_type(L,-1)!=LUA_TSTRING&&lua_type(L,-1)!=LUA_TNUMBER)return fallback;
    size_t n=0;const char* s=lua_tolstring(L,-1,&n);if(!s)return fallback;
    std::string msg(s,n>180?180:n);for(auto& ch:msg)if(static_cast<unsigned char>(ch)<32)ch=' ';
    return msg;
}
#endif
inline std::string RunText(const std::string& code,size_t gameCount){
    if(code.size()>65536)return "Script excede 64 KiB.";
#ifdef INFINITY_WITH_LUA
    Budget b;lua_State* L=lua_newstate(Alloc,&b);if(!L)return "Memoria Lua indisponivel.";
    lua_pushcfunction(L,Setup);lua_pushinteger(L,static_cast<lua_Integer>(gameCount));
    int rc=lua_pcall(L,1,0,0);
    if(rc==LUA_OK)rc=luaL_loadbufferx(L,code.data(),code.size(),"script","t");
    if(rc==LUA_OK){lua_sethook(L,Limit,LUA_MASKCOUNT,1000);rc=lua_pcall(L,0,1,0);}
    auto result=rc==LUA_OK?std::string("Lua: ")+Result(L,"concluido."):std::string("Lua erro: ")+Result(L,"execucao recusada.");
    lua_close(L);return result;
#else
    (void)gameCount;return "Runtime Lua nao incluido nesta build.";
#endif
}
inline std::string RunFile(const std::string& path,size_t count){
    std::ifstream f(path.c_str(),std::ios::binary|std::ios::ate);
    if(!f||f.tellg()<0||f.tellg()>65536)return "Script ausente ou acima de 64 KiB.";
    const auto n=static_cast<size_t>(f.tellg());std::string code(n,'\0');f.seekg(0);
    if(n&&!f.read(&code[0],n))return "Falha de leitura do script.";
    return RunText(code,count);
}
}
