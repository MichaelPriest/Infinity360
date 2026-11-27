#include "Modules/Scripts/LuaRuntime.h"
#include <cassert>
int main(int argc,char** argv){
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
    assert(InfinityLua::RunText("return infinity.api_version",library)=="Lua: 4");
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
    auto filtered=InfinityLua::FilterText("return {3,1}",InfinityLua::Library{3});
    assert(filtered.ok&&filtered.indices==std::vector<size_t>({0,2}));
    auto none=InfinityLua::FilterText("return {}",library);assert(none.ok&&none.indices.empty());
    for(const auto* code:{"return {1,1}","return {0}","return {4}","return {1.5}","return {[1]=1,[3]=2}","return {1, extra=2}","return true","while true do end"}){
        auto bad=InfinityLua::FilterText(code,InfinityLua::Library{3});assert(!bad.ok&&bad.indices.empty());
    }
    auto chosen=InfinityLua::FilterText("local r={}; for i=1,infinity.game_count do if infinity.game(i).favorite then r[#r+1]=i end end; return r",library);
    assert(chosen.ok&&chosen.indices==std::vector<size_t>({0}));
    auto ordered=InfinityLua::FilterText("return {3,1,2}",InfinityLua::Library{3},true);
    assert(ordered.ok&&ordered.indices==std::vector<size_t>({2,0,1}));
    auto sorted=InfinityLua::FilterText("local r={1,2,3}; table.sort(r,function(a,b) return a>b end); return r",InfinityLua::Library{3},true);
    assert(sorted.ok&&sorted.indices==std::vector<size_t>({2,1,0}));
    for(const auto* code:{"return {1,2}","return {1,1,3}","return {1,2,4}","return {[1]=1,[3]=3}"}){
        assert(!InfinityLua::FilterText(code,InfinityLua::Library{3},true).ok);
    }

    using namespace InfinityLua;
    assert(StorageLabel("Usb0:\\Games\\title\\default.xex")=="Usb0:");
    assert(StorageLabel("Hdd1:/Content/0000000000000000")=="Hdd1:");
    assert(StorageLabel("/tmp/test:name/default.xex")=="Local");
    assert(StorageLabel("relative/path")=="Local");
    const std::string formatter="return function(i) if i == 2 then return nil end return 'Jogo '..i end";
    auto page=SubtitlePage(formatter,Library{300},{129,1,0});
    assert(page.error.empty() && page.lines==std::vector<std::string>({"Jogo 130","","Jogo 1"}));
    assert(SubtitlePage(formatter,Library{},{}).error.empty());
    assert(!SubtitlePage(formatter,library,{1}).error.empty());
    assert(!SubtitlePage(formatter,library,std::vector<size_t>(MaxSubtitleRows+1,0)).error.empty());
    assert(!SubtitlePage(std::string(MaxSourceBytes+1,'x'),library,{}).error.empty());
    for(const auto* bad:{"return true","return function(i) if i==1 then return 'ok' end return 3 end",
                         "return function() return {} end","return function() return 'a\\0b' end",
                         "return function() return 'a\\nb' end","return function() error('recusado') end",
                         "return function() while true do end end","return function() local t={} while true do t[#t+1]={} end end"}){
        auto invalid=SubtitlePage(bad,Library{2},{0,1});
        assert(!invalid.error.empty() && invalid.lines.empty());
    }
    const auto literal=[](size_t n){return "return function() return '"+std::string(n,'a')+"' end";};
    assert(SubtitlePage(literal(MaxSubtitleBytes),library,{0}).lines.at(0).size()==MaxSubtitleBytes);
    assert(!SubtitlePage(literal(MaxSubtitleBytes+1),library,{0}).error.empty());
    reads=0;
    auto cached=SubtitlePage("return function(i) local a=infinity.metadata(i); return infinity.metadata(i).title_id_hex end",meta,{129,2});
    assert(cached.error.empty() && cached.lines==std::vector<std::string>({"4D5307E6","4D5307E6"}) && reads==2);
    assert(argc==2);
    const Library examples{3,nullptr,[](const void*,size_t i)->GameView{
        static const std::string xex="Usb1:",pkg="Pacote XBLA";
        return {nullptr,i==1?&pkg:&xex,false,i==1,false,false,i==1?"Hdd1:":"Usb1:"};
    },[](const void*,size_t i)->MetadataView{
        return {i!=2,0xF1234567u,0,0x2F1234ABu};
    }};
    auto example=[&](const char* file){
        std::string source;
        assert(ReadSource(std::string(argv[1])+"/"+file,source).empty());
        auto result=SubtitlePage(source,examples,{1,0,2});
        assert(result.error.empty());
        return result.lines;
    };
    assert(example("Title-ID.lua")==std::vector<std::string>({"Title ID F1234567","Title ID F1234567",""}));
    assert(example("Versao.lua")==std::vector<std::string>({"v2.15.4660.171","v2.15.4660.171",""}));
    assert(example("Dispositivo.lua")==std::vector<std::string>({"Hdd1:","Usb1:","Usb1:"}));
    assert(example("Tipo.lua")==std::vector<std::string>({"Pacote XBLA","XEX","XEX"}));
    Library compat{3,nullptr,[](const void*,size_t i)->GameView{
        static const std::string names[]={"Alpha","Beta","Gamma"};
        return {&names[i],nullptr,false,false,false,false,"Local",unsigned(i+1)};
    },[](const void*,size_t i)->MetadataView{return {true,unsigned(3-i),unsigned(i+1),0,0, unsigned(i+1),3};}};
    auto legacy=FilterText("GameListFilterCategories.User['Group']=function(c) return c.Group==ContentGroup.Xbox360 or c.Group==ContentGroup.XBLA end",compat);
    assert(legacy.ok&&legacy.indices==std::vector<size_t>({0,1}));
    legacy=FilterText("GameListFilterCategories.User['Disc']=function(c) return c.DiscNum<=1 end",compat);
    assert(legacy.ok&&legacy.indices==std::vector<size_t>({0}));
    legacy=FilterText("GameListFilterCategories.User['Name']=function(c) return string.lower(c.Name)=='beta' end",compat);
    assert(legacy.ok&&legacy.indices==std::vector<size_t>({1}));
    legacy=FilterText("GameListSorters['ID']=function(a,b,t) return a.TitleId<b.TitleId end",compat,true);
    assert(legacy.ok&&legacy.indices==std::vector<size_t>({2,1,0}));
    for(const auto* script:{"GameListFilterCategories.User['x']=function(c) return c.Missing==0 end",
                            "GameListFilterCategories.User['x']=function(c) return true end; GameListFilterCategories.User['y']=function(c) return false end",
                            "GameListFilterCategories.Other={a=function(c) return true end}",
                            "GameListFilterCategories.User['x']=function(c) while true do end end"}){
        assert(!FilterText(script,compat).ok);
    }
    assert(RunText("return type(string.dump)..type(load)..type(io)",0)=="Lua: nilnilnil");
}
