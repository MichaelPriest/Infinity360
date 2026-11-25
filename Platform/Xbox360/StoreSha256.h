#pragma once
#include <stdint.h>
#include <cstddef>
#include <string>
#include <cstring>
#include <cstdio>
class StoreSha256 {
public:
    StoreSha256();
    void Update(const unsigned char* data,size_t size);
    std::string Finish(); // consumes the state
private:
    uint32_t state_[8];
    unsigned char block_[64];
    unsigned used_;
    uint64_t bytes_;
    void Compress();
};
namespace StoreHashDetail {
inline uint32_t rotr(uint32_t n,unsigned s){return (n>>s)|(n<<(32-s));}
static const uint32_t K[]={
0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
}
inline StoreSha256::StoreSha256():used_(0),bytes_(0) {
    const uint32_t init[]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    std::memcpy(state_,init,sizeof init);
}
inline void StoreSha256::Compress() {
    uint32_t w[64];
    for(unsigned i=0;i<16;++i) {
        const unsigned char* p=block_+4*i;
        w[i]=(uint32_t(p[0])<<24)|(uint32_t(p[1])<<16)|(uint32_t(p[2])<<8)|p[3];
    }
    for(unsigned i=16;i<64;++i) {
        uint32_t a=w[i-15],b=w[i-2];
        w[i]=w[i-16]+(StoreHashDetail::rotr(a,7)^StoreHashDetail::rotr(a,18)^(a>>3))+w[i-7]+(StoreHashDetail::rotr(b,17)^StoreHashDetail::rotr(b,19)^(b>>10));
    }
    uint32_t a=state_[0],b=state_[1],c=state_[2],d=state_[3],e=state_[4],f=state_[5],g=state_[6],h=state_[7];
    for(unsigned i=0;i<64;++i) {
        uint32_t t=h+(StoreHashDetail::rotr(e,6)^StoreHashDetail::rotr(e,11)^StoreHashDetail::rotr(e,25))+((e&f)^((~e)&g))+StoreHashDetail::K[i]+w[i];
        uint32_t u=(StoreHashDetail::rotr(a,2)^StoreHashDetail::rotr(a,13)^StoreHashDetail::rotr(a,22))+((a&b)^(a&c)^(b&c));
        h=g;g=f;f=e;e=d+t;d=c;c=b;b=a;a=t+u;
    }
    state_[0]+=a;state_[1]+=b;state_[2]+=c;state_[3]+=d;state_[4]+=e;state_[5]+=f;state_[6]+=g;state_[7]+=h;
}
inline void StoreSha256::Update(const unsigned char* data,size_t size) {
    bytes_+=size;
    while(size) {
        size_t n=64-used_;if(n>size)n=size;
        std::memcpy(block_+used_,data,n);used_+=static_cast<unsigned>(n);data+=n;size-=n;
        if(used_==64){Compress();used_=0;}
    }
}
inline std::string StoreSha256::Finish() {
    const uint64_t bits=bytes_*8;
    unsigned char pad[128];std::memset(pad,0,sizeof pad);pad[0]=0x80;
    const unsigned n=used_<56?56-used_:120-used_;
    for(unsigned i=0;i<8;++i)pad[n+i]=static_cast<unsigned char>(bits>>(56-i*8));
    Update(pad,n+8);
    char hex[65];
    for(unsigned i=0;i<8;++i)std::sprintf(hex+i*8,"%08lx",static_cast<unsigned long>(state_[i]));
    return hex;
}
