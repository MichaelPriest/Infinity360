#include "http_range.h"
int main(){
    CloudRange::Range r;
    if(!CloudRange::Parse("bytes 0-255/1024",r)||!CloudRange::Matches(r,0,255,1024))return 1;
    if(!CloudRange::Parse("bytes 900-1023/1024",r)||!CloudRange::Matches(r,900,1200,1024))return 2;
    if(CloudRange::Matches(r,899,1200,1024)||CloudRange::Matches(r,900,1200,2048))return 3;
    for(const auto* s:{"bytes 0-1/*","bytes 1-0/2","bytes 0-2/2","bytes 0-1/999999999999999999999","bytes 0-1/2 "})
        if(CloudRange::Parse(s,r))return 4;
    if(!CloudRange::StrongTag("\"original\"")||CloudRange::StrongTag("W/\"weak\"")||CloudRange::StrongTag("\"x\r\ny\""))return 5;
    return 0;
}
