#version 450 core

layout (location = 0) out vec4 gPosition; //   a: roughness
layout (location = 1) out vec4 gNormal;   //   a: metalness
layout (location = 2) out vec4 gAlbedo;   // rgb: albedo,   a: ao

smooth in vec4 Position;
smooth in vec4 Normal;
smooth in vec4 Color;

void main()
{
    gPosition = Position;
    gNormal = Normal;
    gAlbedo = Color;
}
