#version 450 core

layout (location = 0) out vec4 gPosition; //   a: roughness
layout (location = 1) out vec4 gNormal;   //   a: metalness
layout (location = 2) out vec4 gAlbedo;   // rgb: albedo,   a: ao

// ------------------------------------------------------------------------

in vec4 Position;
in vec4 Normal;
in vec4 Color;

uniform vec3 eye;
uniform int seed;

// ------------------------------------------------------------------------

float rand( inout uint f) 
{
    f = (f ^ 61) ^ (f >> 16);
    f *= 9;
    f = f ^ (f >> 4);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15);
    return fract(float(f) * 2.3283064e-10);
}

float randBi(inout uint s)
{
    return rand(s) * 2.0 - 1.0;
}

void main()
{
    const vec3 V = eye - Position.xyz;
    if(dot(V, Normal.xyz) < 0.0)
    {
        discard;
        return;
    }

    uint s = uint(seed) 
        ^ uint(gl_FragCoord.x * 39163.0) 
        ^ uint(gl_FragCoord.y * 64601.0);

    const vec2 coord = gl_PointCoord * 2.0 - 1.0;
    const float val = 2.0 * rand(s);
    if(length(coord) > sqrt(val))
    {
        discard;
        return;
    }

    gPosition = Position;
    gNormal = Normal;
    gAlbedo = Color;
}
