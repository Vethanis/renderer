#version 450 core

layout(location = 0) in vec4 p;
layout(location = 1) in vec4 n;
layout(location = 2) in vec4 t;

out mat3 TBN;
out vec3 P;
out vec2 UV;
flat out int MID;

uniform mat4 MVP;
uniform mat4 M;
uniform mat3 IM;

void main() {
    gl_Position = MVP * vec4(p.xyz, 1.0);
    P = vec3(M * vec4(p.xyz, 1.0));
    UV = vec2(p.w, n.w);
    MID = int(t.w);
    vec3 T = normalize(vec3(M * vec4(IM * t.xyz, 0.0)));
    vec3 N = normalize(vec3(M * vec4(IM * n.xyz, 0.0)));
    vec3 B = cross(T, N);
    TBN = mat3(T, B, N);
}
