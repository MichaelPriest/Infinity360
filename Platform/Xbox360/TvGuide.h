#pragma once
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

struct TvPlaybackInfo {
    std::string channel,programme,next;
    bool favorite=false;
};
struct TvProgramme {
    int64_t start, stop;
    std::string title;
};
class TvGuide {
public:
    typedef std::vector<TvProgramme> Schedule;
    enum Status { Absent, Ready, Limited, Invalid };
    TvGuide() : status_(Absent) {}
    Status Load(const std::string& path);
    Status State() const { return status_; }
    const Schedule& For(const std::string& id,const std::string& fallback) const;
    static const TvProgramme* Current(const Schedule& list,int64_t now);
    static const TvProgramme* Next(const Schedule& list,int64_t now);
private:
    std::map<std::string,Schedule> channels_;
    std::map<std::string,std::string> aliases_;
    Status status_;
};
