#version 450 core

layout(location = 0) in vec4 p;
layout(location = 1) in vec4 n;

out vec3 MacroNormal;
out vec3 P;

uniform mat4 MVP;
uniform mat4 M;
uniform mat3 IM;

void main() {
    gl_Position = MVP * vec4(p.xyz, 1.0);
    P = vec3(M * vec4(p.xyz, 1.0));
    MacroNormal = normalize(vec3(M * vec4(IM * n.xyz, 0.0)));
}
