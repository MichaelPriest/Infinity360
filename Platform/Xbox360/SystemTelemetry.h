#pragma once
#include <cstdint>
#ifdef __XSDK__
#include "xsdk/kernel.h"
#endif

// Read-only kernel snapshot. Keep title/system counters separate: their
// semantics may differ and summing them can double-count physical pages.
struct SystemTelemetry {
    bool available=false;
    uint64_t physical=0,kernel=0,titlePhysical=0,titleAvailable=0,systemPhysical=0;
    uint64_t titleHeap=0,titleStack=0,titleImage=0;
    static SystemTelemetry FromPages(uint32_t total,uint32_t kernelPages,uint32_t titlePages,
                                    uint32_t titleFree,uint32_t systemPages,uint32_t heap,
                                    uint32_t stack,uint32_t image){
        SystemTelemetry result;
        if(!total||total>1048576u||kernelPages>total||titlePages>total||
           titleFree>total||systemPages>total||heap>total||stack>total||image>total)return result;
        constexpr uint64_t PageBytes=4096;
        result.available=true;
        result.physical=total*PageBytes;result.kernel=kernelPages*PageBytes;
        result.titlePhysical=titlePages*PageBytes;result.titleAvailable=titleFree*PageBytes;
        result.systemPhysical=systemPages*PageBytes;result.titleHeap=heap*PageBytes;
        result.titleStack=stack*PageBytes;result.titleImage=image*PageBytes;
        return result;
    }
    static SystemTelemetry Read(){
#ifdef __XSDK__
        MM_QUERY_STATISTICS_RESULT stats{};stats.size=sizeof(stats);
        if(NT_SUCCESS(MmQueryStatistics(&stats)))
            return FromPages(stats.total_physical_pages,stats.kernel_pages,stats.title.physical_pages,
                stats.title.available_pages,stats.system.physical_pages,stats.title.heap_pages,
                stats.title.stack_pages,stats.title.image_pages);
#endif
        return {};
    }
};
