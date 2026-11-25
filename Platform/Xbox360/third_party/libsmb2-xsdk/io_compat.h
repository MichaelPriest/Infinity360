#pragma once
/* Socket-only vector I/O: one nonempty vector per syscall. Short successful
 * operations are legal for a TCP stream and the library advances its iovecs.
 * Never buffer an entire untrusted PDU or emulate crypto/entropy. */
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#if defined(INFINITY_SMB_HAS_IOVEC)
#include <sys/uio.h>
#else
struct iovec { void *iov_base; size_t iov_len; };
#endif
static inline ssize_t infinity_smb_readv(int fd,const struct iovec *v,int count){
    if(count<0||(!v&&count)){errno=EINVAL;return -1;}
    for(int i=0;i<count;++i)if(v[i].iov_len)
        return recv(fd,v[i].iov_base,v[i].iov_len,0);
    return 0;
}
static inline ssize_t infinity_smb_writev(int fd,const struct iovec *v,int count){
    if(count<0||(!v&&count)){errno=EINVAL;return -1;}
    for(int i=0;i<count;++i)if(v[i].iov_len)
        return send(fd,v[i].iov_base,v[i].iov_len,0);
    return 0;
}
/* No OS account database on the console; explicit credentials are required. */
static inline int infinity_smb_getlogin_r(char *name,size_t size){
    if(name&&size)name[0]=0;
    return ENXIO;
}
#define readv infinity_smb_readv
#define writev infinity_smb_writev
#define getlogin_r infinity_smb_getlogin_r
