
#define _CRT_SECURE_NO_WARNINGS

#include "profiler.h"

#if PROFILING_ENABLED

#include <cstdio>
#include "array.h"
#include "myglheaders.h"
#include "namestore.h"
#include "hashstring.h"
#include <cassert>

Profiler Profiler::ms_instance;

ProfilerEvent::ProfilerEvent(const char* symbol)
{
    HashString hsym(symbol);
    m_symbol = hsym.m_hash;
    ProfilerEvent& event = Profiler::CreateEvent();
    m_handle = event.m_handle;
    event.m_symbol = m_symbol;

    glBeginQuery(GL_TIME_ELAPSED, m_handle); DebugGL();
}

ProfilerEvent::~ProfilerEvent()
{
    glEndQuery(m_handle); DebugGL();
}

 ProfilerEvent& Profiler::CreateEvent()
 {
    ProfilerEvent& event = ms_instance.m_events.grow();
    event.m_handle = ms_instance.m_queries.grow();
    return event;
}

void Profiler::Init()
{
    glGenQueries(QueryCapacity, ms_instance.m_queries.begin()); DebugGL();
}

void Profiler::Deinit()
{
    glDeleteQueries(QueryCapacity, ms_instance.m_queries.begin()); DebugGL();
}

void Profiler::EndFrame()
{
    for(const ProfilerEvent& event : ms_instance.m_events)
    {
        int nanoseconds = -1;
        glGetQueryObjectiv(event.m_handle, GL_QUERY_RESULT, &nanoseconds); DebugGL();
        if(nanoseconds > -1)
        {
            Statistic* pStat = ms_instance.m_statistics[event.m_symbol];
            if(!pStat)
            {
                ms_instance.m_statistics.insert(event.m_symbol, {});
                pStat = ms_instance.m_statistics[event.m_symbol];
            }

            const s32 microseconds = nanoseconds / 1000;
            pStat->m_count++;
            pStat->m_total += microseconds;
            if(pStat->m_maximum < microseconds)
                pStat->m_maximum = microseconds;
            if(pStat->m_minimum > microseconds)
                pStat->m_minimum = microseconds;
        }
    }
    
    ms_instance.m_queries.clear();
    ms_instance.m_events.clear();
}

void Profiler::PrintToFile(const char* filename)
{
    assert(filename);
    
    FILE* pFile = fopen(filename, "wb");
    assert(pFile);
    fprintf(pFile, "%-48s, %-22s, %-22s, %-22s, %-22s, %-22s\r\n", 
        "Name", 
        "total", 
        "average", 
        "min", 
        "max", 
        "count");

    for(unsigned i = 0; i < ms_instance.m_statistics.getCapacity(); ++i)
    {
        if(!ms_instance.m_statistics.validSlot(i))
            continue;

        const unsigned hash = ms_instance.m_statistics.getKey(i);
        const char* name = g_NameStore[hash];
        assert(name);
        const auto& elem = ms_instance.m_statistics.getValue(i);
        const s64 average_time = elem.m_total / elem.m_count;

        fprintf(pFile,"%-48s, %-22lli, %-22lli, %-22lli, %-22lli, %-22lli\r\n", 
            name,
            elem.m_total,
            average_time,
            elem.m_minimum,
            elem.m_maximum,
            elem.m_count
        );
    }

    ms_instance.m_statistics.clear();

    fclose(pFile);
}

#endif