#version 430 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gMaterial;

in vec3 fragPos;
in vec3 fragNorm;
in vec2 fragUv;
in int  fragChannel;

uniform sampler2D albedoSampler0;
uniform sampler2D normalSampler0;
uniform sampler2D albedoSampler1;
uniform sampler2D normalSampler1;
uniform sampler2D albedoSampler2;
uniform sampler2D normalSampler2;
uniform sampler2D albedoSampler3;
uniform sampler2D normalSampler3;

void main(){
    gPosition = fragPos;

    vec3 N = normalize(fragNorm);

    vec3 nsamp;
    vec3 albedo;
    switch(fragChannel){
        case 0:
        {
            albedo = texture(albedoSampler0, fragUv).rgba;
            nsamp = normalize(texture(normalSampler0, fragUv).rgb) * 2.0f - 1.0f;
        }
        break;
        case 1:
        {
            albedo = texture(albedoSampler1, fragUv).rgba;
            nsamp = normalize(texture(normalSampler1, fragUv).rgb) * 2.0f - 1.0f;
        }
        break;
        case 2:
        {
            albedo = texture(albedoSampler2, fragUv).rgba;
            nsamp = normalize(texture(normalSampler2, fragUv).rgb) * 2.0f - 1.0f;
        }
        break;
        case 3:
        {
            albedo = texture(albedoSampler3, fragUv).rgba;
            nsamp = normalize(texture(normalSampler3, fragUv).rgb) * 2.0f - 1.0f;
        }
        break;
    }

    N.xy += nsamp.xy;
    N.z *= nsamp.z;
    gMaterial.rgba = albedo;
    gNormal = normalize(N);
}
