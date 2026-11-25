#include "SmbReadSession.h"
#include <unistd.h>
#include <cstdio>
#include <cerrno>

// No server access, login, GPU or filesystem writes. Link/entropy diagnostic.
static int Entropy(void* buffer,size_t size){
    auto* p=static_cast<unsigned char*>(buffer);
    while(size){
        const size_t n=size>256?256:size;
        if(getentropy(p,n)!=0)return errno?errno:EIO;
        p+=n;size-=n;
    }
    return 0;
}
int main(){
    smb2_set_random_provider(Entropy);
    auto* ctx=smb2_init_context();
    if(!ctx){std::puts("INFINITY SMB: context/entropy unavailable");return 1;}
    smb2_destroy_context(ctx);
    SmbReadSession session;
    // Exercise validation without opening a network socket.
    if(session.Start("8.8.8.8","Public","user","","test"))return 2;
    std::puts("INFINITY SMB: context and local validation completed");
    return 0;
}
