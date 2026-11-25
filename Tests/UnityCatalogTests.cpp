#include "UnityCatalog.h"
#include <cassert>
int main(){
    XboxExecutionInfo info{};info.titleId=0x4d5307e6;info.baseVersion=0x20010002;
    assert(UnityCatalog::UpdatesUrl(info)=="https://xboxunity.net/api/tu/4D5307E6/20010002");
    std::vector<unsigned> covers;
    assert(UnityCatalog::CoversUrl(0x4D5307E6)=="https://xboxunity.net/api/Covers/4D5307E6");
    assert(UnityCatalog::ParseCovers(R"([{"titleid":1,"imageid":7},{"titleid":2,"imageid":8}])",1,covers)&&covers.size()==1&&covers[0]==7);
    assert(!UnityCatalog::ParseCovers(R"([{"titleid":1,"imageid":7,"imageid":8}])",1,covers));
    info.baseVersion=0;assert(UnityCatalog::UpdatesUrl(info).empty());
    std::vector<UnityUpdate> rows;
    assert(UnityCatalog::ParseUpdates(R"([{"TitleUpdateID":7,"version":"2","filesize":"12345","url":"http://untrusted.example/a","other":{"x":[1,2]}}])",rows));
    assert(rows.size()==1&&rows[0].id==7&&rows[0].version=="2"&&rows[0].bytes==12345&&rows[0].url.empty());
    assert(UnityCatalog::FileUrl("http://xboxunity.net/api/file/7")=="https://xboxunity.net/api/file/7");
    assert(UnityCatalog::FileUrl("https://xboxunity.net.evil.example/file").empty());
    assert(UnityCatalog::FileUrl("https://xboxunity.net:444/file").empty());
    assert(UnityCatalog::FileUrl("https://xboxunity.net/file#fragment").empty());
    std::string md5;
    assert(UnityCatalog::ParseMd5(R"({"md5":"D41D8CD98F00B204E9800998ECF8427E"})",md5)&&md5=="d41d8cd98f00b204e9800998ecf8427e");
    assert(!UnityCatalog::ParseMd5(R"({"md5":"invalid"})",md5));
    assert(UnityCatalog::ParseUpdates(R"([{"TitleUpdateID":7,"filesize":7000,"url":"http://xboxunity.net/api/file/7"}])",rows)&&rows[0].url=="https://xboxunity.net/api/file/7");
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
