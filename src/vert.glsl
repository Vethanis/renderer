#version 450 core

layout(location = 0) in vec3 position;

out vec3 Position;

uniform mat4 MVP;
uniform mat4 M;

void main() 
{
    gl_Position = MVP * vec4(position.xyz, 1.0);
    Position.xyz = vec3(M * vec4(position.xyz, 1.0));
}
