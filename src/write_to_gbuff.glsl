#version 430 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gMaterial;

in vec3 fragPos;
in vec3 fragNorm;
in vec2 fragUv;

uniform sampler2D albedoSampler;
uniform sampler2D normalSampler;
uniform sampler2D specularSampler;

void main(){
    gPosition = fragPos;

    vec3 N = normalize(fragNorm);
    vec3 nsamp = normalize(texture(normalSampler, fragUv).rgb) * 2.0f - 1.0f;
    N.xy += nsamp.xy;
    N.z *= nsamp.z;
    gNormal = normalize(N);

    gMaterial.rgb = texture(albedoSampler, fragUv).rgb;
    gMaterial.a = texture(specularSampler, fragUv).r;
}
