#include "StorePackage.h"
#include "MediaUrl.h"
#include "SavedFile.h"
#include <cstring>
#include <cstdio>
#include <fstream>
#include <sstream>
#if defined(_XBOX)
#include <xtl.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#endif
namespace {
uint32_t rotr(uint32_t n,unsigned s){return (n>>s)|(n<<(32-s));}
const uint32_t K[]={
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
std::string lower(std::string s){for(size_t i=0;i<s.size();++i)if(s[i]>='A'&&s[i]<='Z')s[i]+='a'-'A';return s;}
}
StoreSha256::StoreSha256():used_(0),bytes_(0) {
    const uint32_t init[]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    std::memcpy(state_,init,sizeof init);
}
void StoreSha256::Compress() {
    uint32_t w[64];
    for(unsigned i=0;i<16;++i) {
        const unsigned char* p=block_+4*i;
        w[i]=(uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];
    }
    for(unsigned i=16;i<64;++i) {
        uint32_t a=w[i-15],b=w[i-2];
        w[i]=w[i-16]+(rotr(a,7)^rotr(a,18)^(a>>3))+w[i-7]+(rotr(b,17)^rotr(b,19)^(b>>10));
    }
    uint32_t a=state_[0],b=state_[1],c=state_[2],d=state_[3],e=state_[4],f=state_[5],g=state_[6],h=state_[7];
    for(unsigned i=0;i<64;++i) {
        uint32_t t=h+(rotr(e,6)^rotr(e,11)^rotr(e,25))+((e&f)^((~e)&g))+K[i]+w[i];
        uint32_t u=(rotr(a,2)^rotr(a,13)^rotr(a,22))+((a&b)^(a&c)^(b&c));
        h=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+u;
    }
    state_[0]+=a;state_[1]+=b;state_[2]+=c;state_[3]+=d;state_[4]+=e;state_[5]+=f;state_[6]+=g;state_[7]+=h;
}
void StoreSha256::Update(const unsigned char* data,size_t size) {
    bytes_+=size;
    while(size) {
        size_t n=64-used_;if(n>size)n=size;
        std::memcpy(block_+used_,data,n);used_+=static_cast<unsigned>(n);data+=n;size-=n;
        if(used_==64){Compress();used_=0;}
    }
}
std::string StoreSha256::Finish() {
    const uint64_t bits=bytes_*8;
    unsigned char pad[128];std::memset(pad,0,sizeof pad);pad[0]=0x80;
    const unsigned n=used_<56?56-used_:120-used_;
    for(unsigned i=0;i<8;++i)pad[n+i]=static_cast<unsigned char>(bits>>(56-i*8));
    Update(pad,n+8);
    char hex[65];
    for(unsigned i=0;i<8;++i)std::sprintf(hex+i*8,"%08lx",static_cast<unsigned long>(state_[i]));
    return hex;
}
bool StoreNativeSource(const StoreItem& item,const std::string& root) {
    MediaUrl url;
    if(!url.Parse(item.url)||!url.secure||url.port!=443||item.url.find('#')!=std::string::npos)return false;
    // Explicit local allowlist of exact DNS names, no wildcard.
    std::ifstream in((root+"infinity360-store-hosts.txt").c_str(),std::ios::binary);
    char bytes[8193];in.read(bytes,sizeof bytes);
    if(in.bad()||in.gcount()>=static_cast<std::streamsize>(sizeof bytes))return false;
    std::istringstream lines(std::string(bytes,static_cast<size_t>(in.gcount())));
    std::string line;unsigned count=0;bool match=false;
    while(std::getline(lines,line)) {
        if(!line.empty()&&line[line.size()-1]=='\r')line.erase(line.size()-1);
        if(line.empty()||line[0]=='#')continue;
        if(++count>32||line.size()>253||line.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-")!=std::string::npos)return false;
        if(lower(line)==lower(url.host))match=true;
    }
    return match;
}
StorePackage::StorePackage(const FileWorkspace& workspace):workspace_(workspace),done_(0),active_(false)
#if defined(_XBOX) || defined(_WIN32)
    ,handle_(INVALID_HANDLE_VALUE)
#else
    ,handle_(-1)
#endif
{}
StorePackage::~StorePackage(){Cancel();}
void StorePackage::Cancel() {
#if defined(_XBOX) || defined(_WIN32)
    if(handle_!=INVALID_HANDLE_VALUE){CloseHandle(handle_);handle_=INVALID_HANDLE_VALUE;}
    if(!temp_.empty())DeleteFileA(temp_.c_str());
#else
    if(handle_>=0){close(handle_);handle_=-1;}
    if(!temp_.empty())unlink(temp_.c_str());
#endif
    temp_.clear();active_=false;
}
bool StorePackage::Fail(const char* message){Cancel();error_=message;return false;}
bool StorePackage::Begin(const StoreItem& item,const std::string& root) {
    Cancel();error_.clear();done_=0;hash_=StoreSha256();item_=item;
    if(item.id.empty()||item.id.size()>32||item.id.find_first_not_of("abcdefghijklmnopqrstuvwxyz0123456789_-")!=std::string::npos||
       item.sha256.size()!=64||item.sha256.find_first_not_of("0123456789abcdef")!=std::string::npos||
       !item.bytes||item.bytes>512ULL*1024*1024)return Fail("Pacote invalido ou acima de 512 MiB.");
    const std::string personal=FileWorkspace::Join(root,"UserFiles");
    folder_=FileWorkspace::Join(personal,"Store");
    if(!workspace_.CanWrite(personal)&&!workspace_.Create(root,"UserFiles"))return Fail("UserFiles indisponivel.");
    if(!workspace_.CanWrite(folder_)&&!workspace_.Create(personal,"Store"))return Fail("Pasta Store indisponivel.");
    if(!workspace_.CanWrite(folder_))return Fail("Destino invalido.");
    target_=FileWorkspace::Join(folder_,item.id+".pkg");
    // Final publication is exclusive even if another process creates the target.
#if defined(_XBOX) || defined(_WIN32)
    if(GetFileAttributesA(target_.c_str())!=INVALID_FILE_ATTRIBUTES)return Fail("Pacote local ja existe.");
#else
    if(access(target_.c_str(),F_OK)==0)return Fail("Pacote local ja existe.");
#endif
    for(unsigned i=0;i<32;++i) {
        char name[32];std::sprintf(name,"AXCOPY-STORE-%04u.part",i);
        const std::string candidate=FileWorkspace::Join(folder_,name);
#if defined(_XBOX) || defined(_WIN32)
        handle_=CreateFileA(candidate.c_str(),GENERIC_WRITE,0,0,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,0);
        if(handle_==INVALID_HANDLE_VALUE)continue;
#else
#ifdef __XSDK__
        handle_=open(candidate.c_str(),O_WRONLY|O_CREAT|O_EXCL,0600);
#else
        handle_=open(candidate.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);
#endif
        if(handle_<0)continue;
#endif
        temp_=candidate;active_=true;return true;
    }
    return Fail("Nao foi possivel reservar arquivo temporario.");
}
bool StorePackage::Write(const unsigned char* bytes,unsigned count) {
    if(!active_||count>32768||count>item_.bytes-done_)return Fail("Tamanho recebido invalido.");
#if defined(_XBOX) || defined(_WIN32)
    DWORD written=0;if(!WriteFile(handle_,bytes,count,&written,0)||written!=count)return Fail("Falha de escrita.");
#else
    size_t at=0;
    while(at<count){ssize_t n=write(handle_,bytes+at,count-at);if(n<0&&errno==EINTR)continue;if(n<=0)return Fail("Falha de escrita.");at+=n;}
#endif
    hash_.Update(bytes,count);done_+=count;return true;
}
bool StorePackage::Finish() {
    if(!active_||done_!=item_.bytes||hash_.Finish()!=item_.sha256)return Fail("Tamanho ou SHA-256 divergente.");
    if(!workspace_.CanWrite(folder_))return Fail("Destino indisponivel.");
#if defined(_XBOX) || defined(_WIN32)
    bool flush=FlushFileBuffers(handle_)!=0;bool closed=CloseHandle(handle_)!=0;handle_=INVALID_HANDLE_VALUE;
    if(!flush||!closed||!MoveFileA(temp_.c_str(),target_.c_str()))return Fail("Falha ao publicar pacote.");
#else
    bool flush=fsync(handle_)==0;bool closed=close(handle_)==0;handle_=-1;
#ifdef __XSDK__
    if(!flush||!closed||!workspace_.PublishStaged(temp_,target_))return Fail("Falha ao publicar pacote sem sobrescrever.");
#else
    if(!flush||!closed||link(temp_.c_str(),target_.c_str())!=0)return Fail("Falha ao publicar pacote.");
    unlink(temp_.c_str());
#endif
#endif
    temp_.clear();active_=false;
    // Snapshot is informational, not a signature. Never replace the package.
    const std::string receipt=FileWorkspace::Join(folder_,item_.id+".state");
    std::ofstream out((receipt+".tmp").c_str(),std::ios::binary|std::ios::trunc);
    out<<item_.sha256<<"\tnative_verified\t"<<done_<<"\n";out.close();
    if(!out||!replaceSavedFile(receipt))error_="Pacote verificado; registro de estado indisponivel.";
    return true;
}
