#version 450 core

layout(location = 0) in vec3 p;
layout(location = 1) in uint n;
layout(location = 2) in uint c;
layout(location = 3) in uint m;

out vec3 MacroNormal;
out vec3 P;
out vec3 Color;
out vec3 Material; // roughness, metalness, ao

uniform mat4 MVP;
uniform mat4 M;
uniform mat3 IM;

vec4 unpackUint(uint value)
{
    vec4 V;
    V.x = float((value >> 24) & 0xff);
    V.y = float((value >> 16) & 0xff);
    V.z = float((value >>  8) & 0xff);
    V.w = float((value >>  0) & 0xff);
    return V / 255.0;
}

void main() 
{
    {
        gl_Position = MVP * vec4(p.xyz, 1.0);
	    gl_PointSize = clamp(50.0 / gl_Position.w, 1.0f, 500.0f);
        P = vec3(M * vec4(p.xyz, 1.0));
    }

    {
        vec4 N = unpackUint(n);
        N.xyz = normalize(N.xyz * 2.0 - 1.0);
        MacroNormal = normalize(vec3(M * vec4(IM * N.xyz, 0.0)));
    }

    Color = unpackUint(c).xyz;
    Material = unpackUint(m).xyz;
}
