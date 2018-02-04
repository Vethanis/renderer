#pragma once

#include "linmath.h"

#define SHARED_UNIFORMS_BINDING 13

struct SharedUniforms
{
    mat4 MVP;
    mat4 IVP;
    mat4 sunMatrix;
    vec4 sunDirection;
    vec4 sunColor; // w -> intensity
    vec4 eye;
    vec4 render_resolution; // zw -> sunNearFar
    vec4 df_translation; // w -> df_pitch;
    vec4 df_scale;
    ivec4 seed_flags; // x -> seed, y -> draw mode
    ivec4 sampler_states; // x -> env_cm; y -> sunDepth;
};

extern SharedUniforms g_sharedUniforms;

void InitializeSharedUniforms();
void NotifySharedUniformsUpdated();
void ShutdownSharedUniforms();
