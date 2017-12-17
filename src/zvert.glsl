#version 450 core

layout(location = 0) in vec4 position;

uniform mat4 MVP;

void main()
{
	gl_Position = MVP * vec4(position.xyz, 1.0);
	gl_PointSize = clamp(100.0 / gl_Position.w, 1.0f, 500.0f);
}
