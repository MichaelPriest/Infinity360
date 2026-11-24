#include "FtpSession.h"
#include <cassert>
#include <chrono>
#include <filesystem>

int main() {
    namespace fs=std::filesystem;
    const auto root=fs::temp_directory_path()/("aurorax-ftp-"+std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(root/"UserFiles"/"Documents");
    fs::create_directories(root/"Games");
    FileWorkspace workspace(root.string());
    const std::string personal=(root/"UserFiles").string();
    const std::string password="Long-test-Pass-123";
    assert(!FtpSession::ValidCredentials("anonymous",password));
    assert(!FtpSession::ValidCredentials("owner","short"));
    FtpSession invalid(workspace,root.string(),"owner",password);
    assert(invalid.Closed());
    FtpSession ftp(workspace,personal,"owner",password);
    assert(ftp.Greeting().substr(0,3)=="220");
    assert(ftp.Command("PWD")=="530 Login required");
    assert(ftp.Command("USER wrong")=="331 Password required");
    assert(ftp.Command("PASS "+password)=="530 Login incorrect");
    assert(ftp.Command("USER owner")=="331 Password required");
    assert(ftp.Command("PASS "+password)=="230 Logged in");
    assert(ftp.Authenticated());
    assert(ftp.Command("PWD")=="257 \"/\"");
    assert(ftp.Command("CWD Documents")=="250 Directory changed");
    assert(ftp.Command("PWD")=="257 \"/Documents\"");
    assert(ftp.Command("CDUP")=="250 Directory changed");
    assert(ftp.Command("CWD ../Games").substr(0,3)=="550");
    assert(ftp.Command("CWD /../Games").substr(0,3)=="550");
    assert(ftp.Command("CWD HDD1:\\Games").substr(0,3)=="550");
    assert(ftp.Command("CWD /Games").substr(0,3)=="550");
    assert(ftp.Command("STOR test.xex").substr(0,3)=="502");
    assert(ftp.Command("PORT 1,2,3,4,5,6").substr(0,3)=="502");
    assert(ftp.Command("RETR test.txt").substr(0,3)=="502");
    assert(ftp.Command("NOOP\r\nQUIT").substr(0,3)=="500");
    assert(ftp.Command(std::string(513,'A')).substr(0,3)=="500");
#ifndef _WIN32
    std::error_code ec;
    fs::create_directory_symlink(root/"Games",root/"UserFiles"/"link",ec);
    if(!ec)assert(ftp.Command("CWD link").substr(0,3)=="550");
#endif
    assert(ftp.Command("QUIT").substr(0,3)=="221" && ftp.Closed());
    FtpSession bad(workspace,personal,"owner",password);
    for(int i=0;i<3;++i) {bad.Command("USER owner");bad.Command("PASS wrong");}
    assert(bad.Closed() && !bad.Authenticated());
    fs::remove_all(root);
}
