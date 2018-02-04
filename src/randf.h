#pragma once

#include "ints.h"

extern u32 g_seed;

inline u32 randu()
{
    u32 f = g_seed;

    f = (f ^ 61u) ^ (f >> 16u);
    f *= 9u;
    f = f ^ (f >> 4u);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15u);
    
    g_seed = f;

    return f;
}

inline double randd()
{
    return double(randu()) / double(0xffffffff);
}

inline float randf() 
{
    return float(randd());
}