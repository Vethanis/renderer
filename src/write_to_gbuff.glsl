#version 450 core

layout (location = 0) out vec4 gPosition; //   a: roughness
layout (location = 1) out vec4 gNormal;   //   a: metalness
layout (location = 2) out vec4 gAlbedo;   // rgb: albedo,   a: ao

in vec3 Position;

uniform vec3 eye;
uniform sampler3D distance_field;

void main()
{
    const vec3 rd = normalize(Position.xyz - eye.xyz);
    vec3 pt = eye;

    gPosition = Position;
    gNormal = Normal;
    gAlbedo = Color;
}
