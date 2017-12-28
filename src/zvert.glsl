#version 450 core

layout(location = 0) in vec3 position;

uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(position.xyz, 1.0);
}
