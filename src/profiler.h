#pragma once

#define PROFILING_ENABLED 0

#if PROFILING_ENABLED

#include "array.h"
#include "store.h"
#include "ints.h"

struct ProfilerEvent
{
    unsigned m_symbol;
    unsigned m_handle;
    ProfilerEvent() { m_symbol = 0; m_handle = 0; }
    ProfilerEvent(const char* symbol);
    ~ProfilerEvent();
};

class Profiler
{
public:
    static ProfilerEvent& CreateEvent();
    static void Init();
    static void Deinit();
    static void EndFrame();
    static void PrintToFile(const char* filename);
private:
    enum eConstants : int
    {
        EventCapacity = 4096,
        QueryCapacity = 4096,
        StatisticCapacity = 1024,
    };

    struct Statistic
    {
        s64 m_maximum = -1;
        s64 m_minimum = 1 << 29;
        s64 m_total = 0;
        s64 m_count = 0;
    };

    Array<unsigned, QueryCapacity> m_queries;
    Array<ProfilerEvent, EventCapacity> m_events;
    Store<Statistic, StatisticCapacity> m_statistics;

    static Profiler ms_instance;
};

#define ProfilerInit() Profiler::Init();
#define ProfilerDeinit() Profiler::Deinit();
#define ProfilerEndFrame() Profiler::EndFrame();
#define FinishProfiling(x) Profiler::PrintToFile(x);

#else

#define ProfilerEvent(x) 
#define ProfilerInit()
#define ProfilerDeinit()
#define ProfilerEndFrame()
#define FinishProfiling(x)

#endif // PROFILING_ENABLED