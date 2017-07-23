#version 430 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gMaterial;

in vec3 fragPos;
in vec3 fragNorm;
in vec2 fragUv;

uniform sampler2D albedoSampler;
uniform sampler2D specularSampler;

void main(){
    gPosition = fragPos;
    gNormal = normalize(fragNorm);
    gMaterial.rgb = texture(albedoSampler, fragUv).rgb;
    gMaterial.a = texture(specularSampler, fragUv).r;
}
