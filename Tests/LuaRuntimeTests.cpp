#include "Modules/Scripts/LuaRuntime.h"
#include <cassert>
int main(){
    assert(InfinityLua::Available());
    assert(InfinityLua::RunText("return infinity.game_count+2",5)=="Lua: 7");
    assert(InfinityLua::RunText("return type(io)..type(os)..type(load)..type(pcall)",0)=="Lua: nilnilnilnil");
    assert(InfinityLua::RunText("while true do end",0).find("limite de instrucoes")!=std::string::npos);
    assert(InfinityLua::RunText("return +",0).find("Lua erro:")==0);
    assert(InfinityLua::RunText(std::string(65537,'x'),0)=="Script excede 64 KiB.");
}
