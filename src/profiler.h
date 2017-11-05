#pragma once

#define PROFILING_ENABLED 0

#if PROFILING_ENABLED

#include "store.h"

class Profiler
{
public:
    static void Observe(unsigned symbol, double duration);
    static void PrintToFile(const char* filename);
private:
    struct Element
    {
        double total_time;
        double min_time;
        double max_time;
        unsigned occurances;
        Element();
        void Observe(double duration);
        bool operator == (const Element& other)const{ return false; }
    };
    enum eConstants : int
    {
        Capacity = 1024,
    };
    static Store<Element, Capacity> m_store;
};

class ProfilerEvent
{
public:
    ProfilerEvent(const char* symbol);
    ~ProfilerEvent();
private:
    double m_begin;
    unsigned m_symbol;
};

#define FinishProfiling(x) Profiler::PrintToFile(x);

#else

#define ProfilerEvent(x) 
#define FinishProfiling(x)

#endif // PROFILING_ENABLED