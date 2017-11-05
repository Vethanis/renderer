
#define _CRT_SECURE_NO_WARNINGS

#include "profiler.h"

#if PROFILING_ENABLED

#include <cstdio>
#include "array.h"
#include "myglheaders.h"
#include "namestore.h"
#include "hashstring.h"
#include <cassert>

Store<Profiler::Element, Profiler::eConstants::Capacity> Profiler::m_store;

Profiler::Element::Element()
{ 
    total_time = 0.0f;
    min_time = 1000000000.0;
    max_time = 0.0f;
    occurances = 0;
}

void Profiler::Element::Observe(double duration)
{
    total_time += duration;
    min_time = min_time < duration ? min_time : duration;
    max_time = max_time > duration ? max_time : duration;
    occurances++;
}

void Profiler::Observe(unsigned symbol, double duration)
{
    Element* pElement = m_store[symbol];
    if(!pElement)
    {
        assert(m_store.full() == false);
        m_store.insert(symbol, {});
        pElement = m_store[symbol];
    }

    pElement->Observe(duration);
}

void Profiler::PrintToFile(const char* filename)
{
    assert(filename);
    
    FILE* pFile = fopen(filename, "wb");
    assert(pFile);
    const char* header = "Name, total, average, min, max, count\r\n";
    fwrite(header, strlen(header), 1, pFile);

    for(unsigned i = 0; i < m_store.getCapacity(); ++i)
    {
        if(!m_store.validSlot(i))
            continue;

        const unsigned hash = m_store.getKey(i);
        const char* name = g_NameStore[hash];
        assert(name);
        const Element& elem = m_store.getValue(i);
        const double average_time = elem.total_time / double(elem.occurances);

        fprintf(pFile,"%s, %f, %f, %f, %f, %u\r\n", 
            name,
            elem.total_time,
            average_time,
            elem.min_time,
            elem.max_time,
            elem.occurances
        );
    }

    fclose(pFile);
}

ProfilerEvent::ProfilerEvent(const char* symbol)
{
    HashString hsym(symbol);
    m_symbol = hsym.m_hash;
    m_begin = glfwGetTime() * 1000.0;
}

ProfilerEvent::~ProfilerEvent()
{
    const double duration = glfwGetTime() * 1000.0 - m_begin;
    Profiler::Observe(m_symbol, duration);
}

#endif