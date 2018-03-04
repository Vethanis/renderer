#version 450 core

layout(location = 0) in vec4 p;

out vec3 P;

uniform mat4 MVP;
uniform mat4 M;

void main() {
    gl_Position = MVP * vec4(p.xyz, 1.0);
    P = vec3(M * vec4(p.xyz, 1.0));
}
