#include "LanPeerProbe.h"
#include <chrono>
#include <thread>
#include <cassert>
int main(){
    using namespace std::chrono;
    LanPeerProbe a,b;
    assert(!a.Start("127.0.0.1","8.8.8.8"));
    assert(!a.Start("127.0.0.1","127.0.0.1"));
    assert(a.Start("127.0.0.1","127.0.0.2"));
    assert(b.Start("127.0.0.2","127.0.0.1"));
    const auto until=steady_clock::now()+seconds(5);
    while(steady_clock::now()<until&&(a.Received()<2||b.Received()<2)){
        a.Tick();b.Tick();std::this_thread::sleep_for(milliseconds(5));
    }
    assert(a.Received()>=2&&b.Received()>=2);
    assert(a.Received()<=a.Sent()&&b.Received()<=b.Sent());
    assert(a.Sent()<=a.Attempts()&&b.Sent()<=b.Attempts());
    assert(a.Attempts()<=5&&b.Attempts()<=5);
    a.Stop();b.Stop();assert(!a.Running()&&!b.Running());
    assert(a.Start("127.0.0.1","127.0.0.2",LanPeerProbe::Host,30));
    assert(b.Start("127.0.0.2","127.0.0.1",LanPeerProbe::Client,30));
    const auto end=steady_clock::now()+seconds(5);
    while(steady_clock::now()<end&&b.Received()<2){a.Tick();b.Tick();std::this_thread::sleep_for(milliseconds(5));}
    assert(b.Received()>=2&&a.Replies()>=2&&a.Attempts()==0&&b.Requests()==0);
    a.Stop();b.Stop();assert(a.Outcome()==LanPeerProbe::Stopped);
    b.Reset();assert(b.Outcome()==LanPeerProbe::NotStarted&&b.Received()==0);
    assert(!a.Start("127.0.0.1","127.0.0.2",LanPeerProbe::Client,45));
}
