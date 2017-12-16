#version 450 core

layout (location = 0) out vec4 gPosition; //   a: roughness
layout (location = 1) out vec4 gNormal;   //   a: metalness
layout (location = 2) out vec4 gAlbedo;   // rgb: albedo,   a: ao

// ------------------------------------------------------------------------

in vec3 MacroNormal;
in vec3 P;
in vec3 Color;
in vec3 Material; // roughness, metalness, ao

// ------------------------------------------------------------------------

void main()
{
    gPosition = vec4(P, Material.x);
    gNormal = vec4(MacroNormal, Material.y);
    gAlbedo = vec4(Color, Material.z);
}
