#version 450 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;
layout(location = 2) in vec4 color;

out vec4 Position;
out vec4 Normal;
out vec4 Color;

uniform mat4 MVP;
uniform mat4 M;
uniform mat3 IM;

void main() 
{
    gl_Position = MVP * vec4(position.xyz, 1.0);
    Position.xyz = vec3(M * vec4(position.xyz, 1.0));
    Position.w = position.w;
    Normal.xyz = normalize(vec3(M * vec4(IM * normal.xyz, 0.0)));
    Normal.w = normal.w;
    Color = color;
}
