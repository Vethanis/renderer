#pragma once

#include "ints.h"
#include <glm/glm.hpp>

inline float randf(u32& s) 
{
    u32 f = s;
    f = (f ^ 61u) ^ (f >> 16u);
    f *= 9u;
    f = f ^ (f >> 4u);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15u);
    s = f;
    return glm::fract(float(f) * 2.3283064e-10f);
}