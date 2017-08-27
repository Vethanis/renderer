#version 450 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gMaterial;

in mat3 TBN;
in vec3 P;
in vec2 UV;
flat in int MID;

uniform sampler2D albedoSampler0;
uniform sampler2D normalSampler0;
uniform sampler2D albedoSampler1;
uniform sampler2D normalSampler1;
uniform sampler2D albedoSampler2;
uniform sampler2D normalSampler2;
uniform sampler2D albedoSampler3;
uniform sampler2D normalSampler3;

uniform vec3 sunDirection;

void main(){
    gPosition = P.xyz;

    vec3 SN = vec3(0.0, 0.0, 1.0);
    vec4 albedo = vec4(0.0);
    switch(MID){
        case 0:
        {
            albedo = texture(albedoSampler0, UV).rgba;
            SN = texture(normalSampler0, UV).rgb;
        }
        break;
        case 1:
        {
            albedo = texture(albedoSampler1, UV).rgba;
            SN = texture(normalSampler1, UV).rgb;
        }
        break;
        case 2:
        {
            albedo = texture(albedoSampler2, UV).rgba;
            SN = texture(normalSampler2, UV).rgb;
        }
        break;
        case 3:
        {
            albedo = texture(albedoSampler3, UV).rgba;
            SN = texture(normalSampler3, UV).rgb;
        }
        break;
    }

    SN = normalize(TBN * normalize(SN * 2.0 - 1.0));

    gMaterial = albedo;
    gNormal = SN;
}
