#include "Modules/Scripts/LuaRuntime.h"
#include <cassert>
int main(){
    assert(InfinityLua::Available());
    assert(InfinityLua::RunText("return infinity.game_count+2",5)=="Lua: 7");
    assert(InfinityLua::RunText("return type(io)..type(os)..type(load)..type(pcall)",0)=="Lua: nilnilnilnil");
    assert(InfinityLua::RunText("while true do end",0).find("limite de instrucoes")!=std::string::npos);
    assert(InfinityLua::RunText("return +",0).find("Lua erro:")==0);
    assert(InfinityLua::RunText(std::string(65537,'x'),0)=="Script excede 64 KiB.");
    std::string name="Game A",device="HDD";
    InfinityLua::GameView view{&name,&device,true,false};
    InfinityLua::Library library{1,&view,[](const void* p,size_t)->InfinityLua::GameView{return *static_cast<const InfinityLua::GameView*>(p);}};
    assert(InfinityLua::RunText("local g=infinity.game(1); return g.name..':'..g.device..':'..tostring(g.favorite)",library)=="Lua: Game A:HDD:true");
    assert(InfinityLua::RunText("return tostring(infinity.game(0))..tostring(infinity.game(2))",library)=="Lua: nilnil");
    assert(InfinityLua::RunText("local g=infinity.game(1); g.name='changed'; return infinity.game(1).name",library)=="Lua: Game A");
    assert(InfinityLua::RunText("return infinity.api_version",library)=="Lua: 1");
    assert(InfinityLua::RunText("local t={}; while true do t[#t+1]={} end",library).find("Lua erro:")==0);
}
