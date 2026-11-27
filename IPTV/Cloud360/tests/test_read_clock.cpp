#include "read_clock.h"
int main(){
    CloudReadClock clock;
    if(clock.Elapsed(100000)!=100000||clock.Ahead()!=150000)return 1;
    if(!clock.Observe(100000,110000)||clock.Elapsed(110000)!=110000)return 2;
    if(!clock.Observe(120000,2120000)||clock.Elapsed(2120000)!=120000||
       clock.Stalls()!=1||clock.Stalled()!=2000000||clock.Ahead()!=300000)return 3;
    int64_t now=2120000;
    for(int i=0;i<20;++i){if(!clock.Observe(now,now+100000))return 4;now+=100000;}
    if(clock.Ahead()!=CloudReadClock::MaxAhead)return 5;
    for(int i=0;i<64*20;++i){if(!clock.Observe(now,now+1000))return 6;now+=1000;}
    if(clock.Ahead()!=CloudReadClock::MinAhead||clock.Observe(now-1,now)||clock.Observe(now,now+CloudReadClock::MaxStall+1))return 7;
    CloudReadClock limit;now=0;
    for(int i=0;i<CloudReadClock::MaxTotal/CloudReadClock::MaxStall;++i){if(!limit.Observe(now,now+CloudReadClock::MaxStall))return 8;now+=CloudReadClock::MaxStall;}
    if(limit.Observe(now,now+100000))return 9;
    CloudReadClock covered;
    if(!covered.Observe(0,100000,60000)||covered.Stalled()!=40000||covered.Elapsed(100000)!=60000)return 10;
    if(!covered.Observe(100000,200000,200000)||covered.Stalled()!=40000)return 11;
    if(covered.Observe(200000,200000+CloudReadClock::MaxStall+1))return 12;
    if(!covered.Observe(200000,201000)||covered.Elapsed(201000)!=161000)return 13;
    return 0;
}
