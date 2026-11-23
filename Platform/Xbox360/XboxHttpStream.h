#pragma once
#include <xtl.h>
#include <xhttp.h>
#include <vector>
#include "MediaUrl.h"
#include "MpegPlayback.h"

class XboxHttpStream : public MediaReader {
public:
    explicit XboxHttpStream(bool (*cancel)());
    ~XboxHttpStream();
    bool Open(const MediaUrl& url);
    int Read(unsigned char* data,unsigned capacity);
    bool Cancelled() const { return cancelled_; }
    const std::string& Error() const { return error_; }
private:
    XboxHttpStream(const XboxHttpStream&);
    XboxHttpStream& operator=(const XboxHttpStream&);
    bool Wait(DWORD status);
    bool Interrupted(DWORD since);
    static void CALLBACK OnStatus(HINTERNET,DWORD_PTR,DWORD,LPVOID,DWORD);
    bool (*cancel_)();
    bool net_,sockets_,http_,cancelled_;
    HINTERNET session_,connection_,request_;
    DWORD status_,received_;
    std::vector<unsigned char> buffer_; // storage must survive cancellation of an asynchronous read
    std::string error_;
};
