#include "UnityCatalog.h"
#include <cassert>
int main(){
    XboxExecutionInfo info{};info.titleId=0x4d5307e6;info.baseVersion=0x20010002;
    assert(UnityCatalog::UpdatesUrl(info)=="https://xboxunity.net/api/tu/4D5307E6/20010002");
    info.baseVersion=0;assert(UnityCatalog::UpdatesUrl(info).empty());
    std::vector<UnityUpdate> rows;
    assert(UnityCatalog::ParseUpdates(R"([{"TitleUpdateID":7,"version":"2","filesize":"12345","url":"http://untrusted.example/a","other":{"x":[1,2]}}])",rows));
    assert(rows.size()==1&&rows[0].id==7&&rows[0].version=="2"&&rows[0].bytes==12345);
    assert(UnityCatalog::ParseUpdates("[]",rows)&&rows.empty());
    assert(!UnityCatalog::ParseUpdates(R"([{"TitleUpdateID":7},{"TitleUpdateID":"7"}])",rows));
    assert(!UnityCatalog::ParseUpdates(R"([{"TitleUpdateID":7,"TitleUpdateID":8}])",rows));
    assert(!UnityCatalog::ParseUpdates(R"([{"TitleUpdateID":4294967296}])",rows));
    assert(!UnityCatalog::ParseUpdates(R"([{"TitleUpdateID":0}])",rows));
    assert(!UnityCatalog::ParseUpdates(R"({"TitleUpdateID":7})",rows));
    assert(!UnityCatalog::ParseUpdates("[",rows));
    assert(!UnityCatalog::ParseUpdates("[] {}",rows));
    assert(!UnityCatalog::ParseUpdates(std::string(256*1024+1,' '),rows));
}
