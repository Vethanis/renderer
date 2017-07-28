#version 450 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gMaterial;

in vec3 fragPos;
in vec3 fragNorm;
in vec2 fragUv;
flat in int  fragChannel;
in mat3 TBN;

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

    vec3 SN = vec3(0.0);
    vec4 albedo = vec4(1.0, 1.0, 1.0, 1.0);
    switch(fragChannel){
        case 0:
        {
            albedo = texture(albedoSampler0, fragUv).rgba;
            SN = texture(normalSampler0, fragUv).rgb;
        }
        break;
        /*
        case 1:
        {
            albedo = texture(albedoSampler1, fragUv).rgba;
            SN = texture(normalSampler1, fragUv).rgb;
        }
        break;
        case 2:
        {
            albedo = texture(albedoSampler2, fragUv).rgba;
            SN = texture(normalSampler2, fragUv).rgb;
        }
        break;
        case 3:
        {
            albedo = texture(albedoSampler3, fragUv).rgba;
            SN = texture(normalSampler3, fragUv).rgb;
        }
        break;
        */
    }

    SN = normalize(TBN * normalize(SN * 2.0 - 1.0));

    gMaterial = albedo;
    gNormal = SN;
}
