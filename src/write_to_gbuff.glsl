#version 430 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gMaterial;

in vec3 fragPos;
in vec3 fragNorm;
in vec2 fragUv;
flat in int  fragChannel;

uniform sampler2D albedoSampler0;
uniform sampler2D normalSampler0;
uniform sampler2D albedoSampler1;
uniform sampler2D normalSampler1;
uniform sampler2D albedoSampler2;
uniform sampler2D normalSampler2;
uniform sampler2D albedoSampler3;
uniform sampler2D normalSampler3;

vec3 calcNormal(vec3 pos, vec3 N, float H){
    vec3 dpdx = dFdx(pos);
    vec3 dpdy = dFdy(pos);

    float dhdx = dFdx(H);
    float dhdy = dFdy(H);

    vec3 r1 = cross(dpdy, N);
    vec3 r2 = cross(N, dpdx);

    vec3 R = (r1 * dhdx + r2 * dhdy) / dot(dpdx, r1);

    return normalize(N - R);
}

void main(){
    gPosition = fragPos;

    vec3 N = normalize(fragNorm);

    float H = 0.0;
    vec4 albedo = vec4(1.0, 1.0, 1.0, 1.0);
    switch(fragChannel){
        case 0:
        {
            albedo = texture(albedoSampler0, fragUv).rgba;
            H = texture(normalSampler0, fragUv).r;
        }
        break;
        case 1:
        {
            albedo = texture(albedoSampler1, fragUv).rgba;
            H = texture(normalSampler1, fragUv).r;
        }
        break;
        case 2:
        {
            albedo = texture(albedoSampler2, fragUv).rgba;
            H = texture(normalSampler2, fragUv).r;
        }
        break;
        case 3:
        {
            albedo = texture(albedoSampler3, fragUv).rgba;
            H = texture(normalSampler3, fragUv).r;
        }
        break;
    }

    gMaterial = albedo;
    gNormal = calcNormal(fragPos, N, H);
}
