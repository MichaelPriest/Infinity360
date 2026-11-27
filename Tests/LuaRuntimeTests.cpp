#include "Modules/Scripts/LuaRuntime.h"
#include "Modules/Scripts/LuaRepository.h"
#include <filesystem>
#include <chrono>
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
    assert(InfinityLua::RunText("return infinity.api_version",library)=="Lua: 5");
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
    const std::string directional="GameListSorters['ID']=function(a,b,t) if t==SortType.Descending then return a.TitleId>b.TitleId end return a.TitleId<b.TitleId end";
    auto ascending=FilterText(directional,compat,true,false),descending=FilterText(directional,compat,true,true);
    assert(ascending.ok&&ascending.indices==std::vector<size_t>({2,1,0}));
    assert(descending.ok&&descending.indices==std::vector<size_t>({0,1,2}));
    auto reverseNative=FilterText("return {3,1,2}",compat,true,true);
    assert(reverseNative.ok&&reverseNative.indices==std::vector<size_t>({1,0,2}));
    auto oldSubtitle=SubtitlePage("GameListSubtitles['ID']=function(c) return string.format('%s %08X',c.Name,c.TitleId) end",compat,{2,0});
    assert(oldSubtitle.error.empty()&&oldSubtitle.lines==std::vector<std::string>({"Gamma 00000001","Alpha 00000003"}));
    for(const auto* script:{"GameListSubtitles['x']=function(c) return c.ReleaseDate end",
                            "GameListSubtitles['x']=function(c) return 'x' end; GameListSubtitles['y']=function(c) return 'y' end",
                            "GameListSubtitles['x']=function(c) while true do end end","GameListSubtitles['x']=function(c) return 1 end"}){
        auto bad=SubtitlePage(script,compat,{0});assert(!bad.error.empty()&&bad.lines.empty());
    }
    namespace fs=std::filesystem;
    const auto scriptsRoot=fs::path(argv[1]).parent_path();
    const auto temp=fs::temp_directory_path()/("infinity-lua-repo-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(temp/"Scripts"/"Filters");
    fs::create_directories(temp/"Scripts"/"Sorts");
    fs::copy(scriptsRoot/"Repository",temp/"Scripts"/"Repository",fs::copy_options::recursive);
    const auto root=temp.string()+"/";
    std::vector<InfinityLuaRepo::Entry> entries;
    assert(InfinityLuaRepo::Load(root,entries)&&entries.size()==3);
    for(const auto& entry:entries){
        assert(InfinityLuaRepo::Status(root,entry)=="Disponivel");
        assert(InfinityLuaRepo::Install(root,entry)=="Script instalado.");
        assert(InfinityLuaRepo::Status(root,entry)=="Instalado");
        assert(InfinityLuaRepo::Remove(root,entry)=="Script removido.");
        assert(InfinityLuaRepo::Status(root,entry)=="Disponivel");
    }
    const auto entry=entries[0];assert(!entry.url.empty()&&InfinityLuaRepo::RemoteUrl(entry.url));
    std::string payload;assert(InfinityLuaRepo::ReadVerified(InfinityLuaRepo::Source(root,entry),entry,payload));
    assert(InfinityLuaRepo::InstallData(root,entry,payload+"x")!="Script instalado.");
    auto wrong=payload;wrong[0]=wrong[0]=='x'?'y':'x';assert(InfinityLuaRepo::InstallData(root,entry,wrong)!="Script instalado.");
    assert(InfinityLuaRepo::InstallData(root,entry,payload)=="Script instalado.");
    const auto dest=InfinityLuaRepo::Destination(root,entry);
    auto newer=entry;const std::string updated="return 42\n";
    newer.bytes=updated.size();newer.sha=InfinityLuaRepo::Hash(updated);
    assert(InfinityLuaRepo::Status(root,newer)=="Atualizacao");
    assert(InfinityLuaRepo::InstallData(root,newer,updated)=="Script atualizado.");
    assert(InfinityLuaRepo::Status(root,newer)=="Instalado"&&InfinityLuaRepo::CanRestore(root,newer));
    auto latest=newer;const std::string newest="return 43\n";
    latest.bytes=newest.size();latest.sha=InfinityLuaRepo::Hash(newest);
    assert(InfinityLuaRepo::InstallData(root,latest,newest)=="Script atualizado.");
    assert(InfinityLuaRepo::Restore(root,latest)=="Script restaurado.");
    assert(InfinityLuaRepo::Status(root,entry)=="Instalado");
    std::string restored;assert(InfinityLuaRepo::ReadVerified(dest,entry,restored)&&restored==payload);
    std::ofstream(dest,std::ios::trunc)<<"custom";
    assert(!InfinityLuaRepo::UpdateAvailable(root,newer));
    assert(InfinityLuaRepo::InstallData(root,newer,updated)!="Script atualizado.");
    assert(InfinityLuaRepo::Restore(root,newer)!="Script restaurado.");
    assert(InfinityLuaRepo::Status(root,entry)=="Local diferente");
    assert(InfinityLuaRepo::Install(root,entry)!="Script instalado.");
    assert(InfinityLuaRepo::Remove(root,entry)!="Script removido.");
    fs::remove(InfinityLuaRepo::Destination(root,entry));
    fs::remove(dest+".r");fs::remove(dest+".old");
    std::ofstream(InfinityLuaRepo::Source(root,entry),std::ios::app)<<"tamper";
    assert(InfinityLuaRepo::Install(root,entry)!="Script instalado.");
    assert(InfinityLuaRepo::Status(root,entry)=="Online");
    assert(!InfinityLuaRepo::RemoteUrl("http://example.org/a.lua")&&!InfinityLuaRepo::RemoteUrl("https://user@example.org/a.lua")&&!InfinityLuaRepo::RemoteUrl("https://example.org:8443/a.lua")&&!InfinityLuaRepo::RemoteUrl("https://example.org/a.lua#x"));
    std::ofstream(temp/"Scripts"/"Repository"/"catalog.tsv",std::ios::trunc)<<"INFINITY-LUA-REPO-V1\nFilters\t../bad.lua\t1\t"<<std::string(64,'a')<<"\n";
    assert(!InfinityLuaRepo::Load(root,entries)&&entries.empty());
    // The bundled pin must match the exact catalog bytes. Remote import is atomic.
    InfinityLuaRepo::Manifest manifest;
    assert(InfinityLuaRepo::ReadManifest(root,manifest));
    std::string original;
    assert(InfinityLuaRepo::ReadBounded((scriptsRoot/"Repository"/"catalog.tsv").string(),32768,original));
    assert(InfinityLuaRepo::ImportManifest(root,manifest,original)=="Catalogo Lua sincronizado.");
    assert(InfinityLuaRepo::Load(root,entries)&&entries.size()==3);
    assert(InfinityLuaRepo::ImportManifest(root,manifest,original+"x")!="Catalogo Lua sincronizado.");
    auto modified=original;modified[0]='X';
    assert(InfinityLuaRepo::ImportManifest(root,manifest,modified)!="Catalogo Lua sincronizado.");
    auto invalid=manifest;invalid.bytes=modified.size();StoreSha256 h;
    h.Update(reinterpret_cast<const unsigned char*>(modified.data()),modified.size());invalid.sha=h.Finish();
    assert(InfinityLuaRepo::ImportManifest(root,invalid,modified)!="Catalogo Lua sincronizado.");
    assert(InfinityLuaRepo::Load(root,entries)&&entries.size()==3);
    std::string empty="INFINITY-LUA-REPO-V2\n";StoreSha256 eh;
    eh.Update(reinterpret_cast<const unsigned char*>(empty.data()),empty.size());
    auto next=manifest;next.bytes=empty.size();next.sha=eh.Finish();
    assert(InfinityLuaRepo::ImportManifest(root,next,empty)=="Catalogo Lua sincronizado.");
    assert(InfinityLuaRepo::Load(root,entries)&&entries.empty());
    fs::remove_all(temp);

}
