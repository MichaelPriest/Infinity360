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
    InfinityLua::GameView view{&name,&device,true,false,true,false};
    InfinityLua::Library library{1,&view,[](const void* p,size_t)->InfinityLua::GameView{return *static_cast<const InfinityLua::GameView*>(p);}};
    assert(InfinityLua::RunText("local g=infinity.game(1); return g.name..':'..g.device..':'..tostring(g.favorite)",library)=="Lua: Game A:HDD:true");
    assert(InfinityLua::RunText("return tostring(infinity.game(0))..tostring(infinity.game(2))",library)=="Lua: nilnil");
    assert(InfinityLua::RunText("local g=infinity.game(1); g.name='changed'; return infinity.game(1).name",library)=="Lua: Game A");
    assert(InfinityLua::RunText("return infinity.api_version",library)=="Lua: 2");
    assert(InfinityLua::RunText("local g=infinity.game(1); return tostring(g.hidden)..tostring(g.pinned)",library)=="Lua: truefalse");
    assert(InfinityLua::RunText("local t={}; while true do t[#t+1]={} end",library).find("Lua erro:")==0);
    unsigned reads=0;
    InfinityLua::Library meta{130,&reads,nullptr,[](const void* p,size_t i)->InfinityLua::MetadataView{
        ++*const_cast<unsigned*>(static_cast<const unsigned*>(p));
        return {i!=1,0x4D5307E6u,0xF1234567u,2,1,1,2,0x7000u,1};
    }};
    assert(InfinityLua::RunText("local m=infinity.metadata(1); return tostring(m.title_id==0x4D5307E6 and m.media_id==0xF1234567 and m.content_type==0x7000 and m.disc_count==2)",meta)=="Lua: true");
    assert(reads==1);
    reads=0;
    assert(InfinityLua::RunText("for i=1,200 do local m=infinity.metadata(1); m.title_id=0 end; return infinity.metadata(1).title_id",meta)=="Lua: 1297287142");
    assert(reads==1);
    reads=0;
    assert(InfinityLua::RunText("local m,e=infinity.metadata(2); return e",meta)=="Lua: cabecalho indisponivel");
    assert(reads==1);
    reads=0;
    assert(InfinityLua::RunText("for i=1,128 do infinity.metadata(i) end; local m,e=infinity.metadata(129); return e",meta)=="Lua: limite de metadados");
    assert(reads==128);
    assert(InfinityLua::RunText("local m,e=infinity.metadata(0); return e",meta)=="Lua: indisponivel");
}
