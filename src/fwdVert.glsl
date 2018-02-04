#version 450 core

layout(location = 0) in vec3 position;

out vec3 Position;

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

layout(std430, binding = SHARED_UNIFORMS_BINDING) buffer SU_BUFFER
{
    SharedUniforms SU;
};

void main() 
{
    const vec3 pos = position.xyz * SU.df_scale.xyz + SU.df_translation.xyz;
    Position.xyz = pos.xyz;
    gl_Position = SU.MVP * vec4(pos.xyz, 1.0);
}
