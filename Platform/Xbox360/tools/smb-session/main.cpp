#include "../../SmbReadSession.h"
#include "../../SmbBrowser.h"
#include <iostream>
#include <thread>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <cerrno>

// Exclusive local staging on Linux; hold the directory descriptor so a
// destination path replacement cannot redirect publication.
class LocalStage {
public:
    ~LocalStage(){Cancel();}
    bool Start(const std::string& folder,const SmbBrowser::CopyPlan& plan){
        Cancel();if(!SmbBrowser::LocalName(plan.name))return false;
        dir_=open(folder.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW);
        if(dir_<0)return false;
        struct stat st{};
        if(fstatat(dir_,plan.name.c_str(),&st,AT_SYMLINK_NOFOLLOW)==0||errno!=ENOENT){Cancel();return false;}
        target_=plan.name;expected_=plan.bytes;
        for(unsigned i=0;i<32;++i){
            const std::string candidate="AXCOPY-SMB-"+std::to_string(i)+".part";
            fd_=openat(dir_,candidate.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
            if(fd_>=0){temp_=candidate;return true;}
            if(errno!=EEXIST)break;
        }
        Cancel();return false;
    }
    bool Write(const uint8_t* data,size_t n){
        if(fd_<0||n>expected_-done_)return false;
        size_t at=0;
        while(at<n){const auto w=write(fd_,data+at,n-at);if(w<0&&errno==EINTR)continue;if(w<=0)return false;at+=size_t(w);}
        done_+=n;return true;
    }
    bool Publish(){
        if(fd_<0||done_!=expected_)return false;
        const bool flushed=fsync(fd_)==0,closed=close(fd_)==0;fd_=-1;
        if(!flushed||!closed||linkat(dir_,temp_.c_str(),dir_,target_.c_str(),0)!=0)return false;
        unlinkat(dir_,temp_.c_str(),0);temp_.clear();return true;
    }
    void Cancel(){
        if(fd_>=0){close(fd_);fd_=-1;}
        if(dir_>=0){if(!temp_.empty())unlinkat(dir_,temp_.c_str(),0);close(dir_);dir_=-1;}
        temp_.clear();target_.clear();done_=expected_=0;
    }
private:
    int fd_=-1,dir_=-1;uint64_t done_=0,expected_=0;std::string temp_,target_;
};

static bool Wait(SmbReadSession& session){
    while(session.Busy()){
        session.Tick();std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    return session.Status()==SmbReadSession::Ready;
}
static bool Load(SmbReadSession& session,SmbBrowser& browser){
    if(!session.Open(browser.Requested())||!Wait(session)){browser.Abort();return false;}
    std::vector<SmbBrowser::Entry> entries;
    for(const auto& e:session.Entries())entries.push_back({e.name,e.size,e.directory});
    return browser.Commit(std::move(entries),session.Truncated());
}
static void Print(const SmbBrowser& browser){
    std::cout<<"/"<<browser.Current()<<"\n";
    for(size_t i=0;i<browser.Rows().size();++i){
        const auto& e=browser.Rows()[i];std::cout<<i+1<<" "<<(e.directory?"DIR ":"FILE ")<<e.size<<" "<<e.name<<"\n";
    }
    if(browser.Truncated())std::cerr<<"Listing limited to 512 entries.\n";
}
static bool Index(const std::string& input,size_t count,size_t& row){
    if(input.empty()||input.size()>4)return false;
    unsigned n=0;for(char c:input){if(c<'0'||c>'9')return false;n=n*10+unsigned(c-'0');}
    if(!n||n>count)return false;row=n-1;return true;
}
int main(int argc,char** argv){
    if(argc==2&&std::string(argv[1])=="--self-test"){
        if(!SmbReadSession::Share("Public")||SmbReadSession::Share("../Public")||
           !SmbReadSession::Path("A/B")||SmbReadSession::Path("A/../B")||
           SmbReadSession::Path("A/")||SmbReadSession::Path("/A")||
           SmbReadSession::Name("..")||SmbReadSession::Name("A\\B"))return 1;
        SmbBrowser b;SmbBrowser::CopyPlan plan;
        if(!b.BeginRoot()||!b.Commit({{"file.bin",4,false},{"folder",0,true},{"..",0,true}},false)||
           b.Rows().size()!=2||!b.Rows()[0].directory||!b.BeginChild()||b.Plan(plan))return 1;
        b.Abort();if(!b.Current().empty()||!b.BeginChild()||!b.Commit({{"test.bin",8,false}},false)||
           !b.Plan(plan)||plan.remote!="folder/test.bin"||plan.bytes!=8||
           !b.BeginParent()||!b.Commit({{"NUL.txt",1,false}},false)||b.Plan(plan))return 1;
        if(SmbBrowser::LocalName("AXCOPY-0.part")||SmbBrowser::LocalName("bad/name"))return 1;
        SmbReadSession s;
        return s.Start("8.8.8.8","Public","user","","example")?1:0;
    }
    if(argc!=5||!isatty(STDIN_FILENO)){
        std::cerr<<"Usage: infinity_smb_session PRIVATE_IP SHARE USER DOMAIN\nInteractive terminal required; use empty DOMAIN for local account.\n";return 2;
    }
    if(!LanPeerProbe::Private(argv[1])||!SmbReadSession::Share(argv[2])||
       !SmbReadSession::Field(argv[3],128)||!SmbReadSession::Field(argv[4],128,true))return 2;
    char* secret=getpass("SMB password: ");if(!secret)return 2;
    std::string password(secret);
    volatile char* raw=secret;for(size_t i=0,n=std::strlen(secret);i<n;++i)raw[i]=0;
    SmbReadSession session;
    const bool started=session.Start(argv[1],argv[2],argv[3],argv[4],password);
    volatile char* mem=password.empty()?nullptr:&password[0];
    for(size_t i=0;i<password.size();++i)mem[i]=0;
    password.clear();
    SmbBrowser browser;
    if(!started||!Wait(session)||!browser.BeginRoot()||!Load(session,browser)){
        std::cerr<<session.Label()<<"\n";return 1;
    }
    Print(browser);
    for(std::string command;std::cout<<"ls | cd NUMBER | up | get NUMBER | quit > "&&std::getline(std::cin,command);){
        if(command=="quit")return 0;
        if(command=="ls"){if(!browser.BeginRefresh()||!Load(session,browser))break;Print(browser);continue;}
        if(command=="up"){if(!browser.BeginParent())continue;if(!Load(session,browser))break;Print(browser);continue;}
        const bool get=command.compare(0,4,"get ")==0,cd=command.compare(0,3,"cd ")==0;
        size_t row=0;if((!get&&!cd)||!Index(command.substr(get?4:3),browser.Rows().size(),row))continue;
        browser.Select(row);
        if(cd){if(!browser.BeginChild())continue;if(!Load(session,browser))break;Print(browser);continue;}
        SmbBrowser::CopyPlan plan;
        if(!browser.Plan(plan)){std::cerr<<"Invalid file or FATX destination name/size.\n";continue;}
        std::string folder,confirmation;
        std::cout<<"Existing local destination directory: ";if(!std::getline(std::cin,folder))return 0;
        std::cout<<"Copy "<<plan.name<<" ("<<plan.bytes<<" bytes) without overwrite? Type YES: ";
        if(!std::getline(std::cin,confirmation))return 0;if(confirmation!="YES")continue;
        LocalStage stage;
        if(!stage.Start(folder,plan)){std::cerr<<"Destination unavailable or occupied.\n";continue;}
        if(!session.ReadFile(plan,[&](const uint8_t* data,size_t n){return stage.Write(data,n);})||
           !Wait(session)||!session.CopyComplete()||!stage.Publish()){
            std::cerr<<"Copy not published: "<<session.Label()<<"\n";
            session.Cancel();return 1;
        }
        std::cout<<"Copy published ("<<session.Done()<<" bytes).\n";
    }
    std::cerr<<session.Label()<<"\n";return 1;
}
