#version 450 core

layout(location = 0) in vec4 p;

uniform mat4 MVP;

void main() {
	gl_Position = MVP * vec4(p.xyz, 1.0);
}
