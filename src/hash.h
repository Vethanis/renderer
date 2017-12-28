#pragma once 

#include "ints.h"

inline u32 fnv(const char* name)
{
    const u8* data = (const u8*)name;
    u32 val = 3759247821;
    while(*data)
    {
        val ^= *data++;
        val *= 0x01000193;
    }
    val &= 0x7fffffff;
    val |= val==0;
    return val;
}

inline u32 fnv(const void* p, const u32 len)
{
    const u8* data = (const u8*)p;
    u32 val = 3759247821;
    for(u32 i = 0; i < len; i++)
    {
        val ^= data[i];
        val *= 0x01000193;
    }
    if(val == 0xffffffff)
    {
        val = 0x7fffffff;
    }
    else if(val == 0)
    {
        val = 1;
    }
    return val;
}
