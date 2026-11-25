#include "../../SmbReadSession.h"
#include <iostream>
#include <thread>
#include <unistd.h>
#include <cstring>

static bool Wait(SmbReadSession& session){
    while(session.Busy()){
        session.Tick();std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return session.Status()==SmbReadSession::Ready;
}
int main(int argc,char** argv){
    if(argc==2&&std::string(argv[1])=="--self-test"){
        if(!SmbReadSession::Share("Public")||SmbReadSession::Share("../Public")||
           !SmbReadSession::Path("A/B")||SmbReadSession::Path("A/../B")||
           SmbReadSession::Path("A/")||SmbReadSession::Path("/A")||
           SmbReadSession::Name("..")||SmbReadSession::Name("A\\B"))return 1;
        SmbReadSession s;
        return s.Start("8.8.8.8","Public","user","","example")?1:0;
    }
    if(argc!=5||!isatty(STDIN_FILENO)){
        std::cerr<<"Usage: infinity_smb_session PRIVATE_IP SHARE USER DOMAIN\nInteractive terminal required; use empty DOMAIN for local account.\n";return 2;
    }
    if(!LanPeerProbe::Private(argv[1])||!SmbReadSession::Share(argv[2])||
       !SmbReadSession::Field(argv[3],128)||!SmbReadSession::Field(argv[4],128,true))return 2;
    char* secret=getpass("SMB password: ");
    if(!secret)return 2;
    std::string password(secret);
    volatile char* raw=secret;for(size_t i=0,n=std::strlen(secret);i<n;++i)raw[i]=0;
    SmbReadSession session;
    const bool started=session.Start(argv[1],argv[2],argv[3],argv[4],password);
    volatile char* mem=password.empty()?nullptr:&password[0];
    for(size_t i=0;i<password.size();++i)mem[i]=0;
    password.clear();
    if(!started||!Wait(session)||!session.Open("")||!Wait(session)){
        std::cerr<<session.Label()<<"\n";return 1;
    }
    for(const auto& item:session.Entries())
        std::cout<<(item.directory?"DIR ":"FILE ")<<item.size<<" "<<item.name<<"\n";
    if(session.Truncated())std::cerr<<"Listing limited to 512 entries.\n";
    return 0;
}
