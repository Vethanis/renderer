#pragma once

#define PROFILING_ENABLED 0


#if PROFILING_ENABLED
// ---------------------------------------------------------------

#define RMT_USE_OPENGL 1
#include "remotery.h"

extern Remotery* g_rmt;

inline void ProfilerInit()
{
    rmt_CreateGlobalInstance(&g_rmt);
    rmt_BindOpenGL();
}

inline void ProfilerDeinit()
{
    rmt_UnbindOpenGL();
    rmt_DestroyGlobalInstance(g_rmt);
}

struct ProfilerEvent
{
    const char* m_sym;
    ProfilerEvent(const char* s)
    {
        m_sym = s;
        rmt_BeginCPUSample(m_sym, 0);
    }
    ~ProfilerEvent()
    {
        rmt_EndCPUSample();
    }
};
struct ProfilerGPUEvent
{
    const char* m_sym;
    ProfilerGPUEvent(const char* s)
    {
        m_sym = s;
        rmt_BeginOpenGLSample(m_sym);
    }
    ~ProfilerGPUEvent()
    {
        rmt_EndOpenGLSample();
    }
};

#else // !PROFILING_ENABLED
// ---------------------------------------------------------------

#define ProfilerInit() 
#define ProfilerDeinit() 
#define ProfilerEvent(x) 
#define ProfilerGPUEvent(x) 

#endif // PROFILING_ENABLED
// ---------------------------------------------------------------