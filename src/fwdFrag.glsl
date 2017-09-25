#version 450 core

out vec4 outColor;

in mat3 TBN;
in vec3 P;
in vec2 UV;
flat in uint MID;

// ------------------------------------------------------------------------

uniform sampler2D albedoSampler0;
uniform sampler2D normalSampler0;
uniform sampler2D albedoSampler1;
uniform sampler2D normalSampler1;
uniform sampler2D albedoSampler2;
uniform sampler2D normalSampler2;
uniform sampler2D albedoSampler3;
uniform sampler2D normalSampler3;

uniform samplerCube env_cm;

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 eye;
uniform int seed;
uniform int draw_flags;
uniform int object_flags;

uniform float iorr;
uniform float roughness_multiplier;

// ------------------------------------------------------------------------

#define DF_DIRECT           0
#define DF_INDIRECT         1
#define DF_NORMALS          2
#define DF_REFLECT          3
#define DF_UV               4
#define DF_DIRECT_CUBEMAP   5
#define DF_VIS_CUBEMAP      6
#define DF_VIS_REFRACT      7

#define ODF_DEFAULT         0
#define ODF_SKY             1

// ------------------------------------------------------------------------

float rand( inout uint f) {
    f = (f ^ 61) ^ (f >> 16);
    f *= 9;
    f = f ^ (f >> 4);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15);
    return fract(float(f) * 2.3283064e-10);
}

float randBi(inout uint s){
    return rand(s) * 2.0 - 1.0;
}

void cosHemiUV(vec3 N, out vec3 u, out vec3 v){
    if(abs(N.x) > 0.1)
        u = cross(vec3(0.0, 1.0, 0.0), N);
    else
        u = cross(vec3(1.0, 0.0, 0.0), N);
    u = normalize(u);
    v = cross(N, u);
}

vec3 cosHemi(vec3 N, vec3 u, vec3 v, inout uint s){
    float r1 = 3.141592 * 2.0 * rand(s);
    float r2 = rand(s);
    float r2s = sqrt(r2);
    return normalize(
        u * cos(r1) * r2s 
        + v * sin(r1) * r2s 
        + N * sqrt(1.0 - r2)
        );
}

void getNormalAndAlbedo(out vec3 N, out vec4 albedo){
    switch(MID){
        default:
        case 0:
        {
            albedo = texture(albedoSampler0, UV).rgba;
            N = texture(normalSampler0, UV).rgb;
        }
        break;
        case 1:
        {
            albedo = texture(albedoSampler1, UV).rgba;
            N = texture(normalSampler1, UV).rgb;
        }
        break;
        case 2:
        {
            albedo = texture(albedoSampler2, UV).rgba;
            N = texture(normalSampler2, UV).rgb;
        }
        break;
        case 3:
        {
            albedo = texture(albedoSampler3, UV).rgba;
            N = texture(normalSampler3, UV).rgb;
        }
        break;
    }
    N = normalize(TBN * normalize(N * 2.0 - 1.0));

    albedo.xyz = pow(albedo.xyz, vec3(2.2));
}

// ------------------------------------------------------------------------

float DisGGX(vec3 N, vec3 H, float roughness){
    const float a = roughness * roughness;
    const float a2 = a * a;
    const float NdH = max(dot(N, H), 0.0);
    const float NdH2 = NdH * NdH;

    const float nom = a2;
    const float denom_term = (NdH2 * (a2 - 1.0) + 1.0);
    const float denom = 3.141592 * denom_term * denom_term;

    return nom / denom;
}

float GeomSchlickGGX(float NdV, float roughness){
    const float r = (roughness + 1.0);
    const float k = (r * r) * 0.125;

    const float nom = NdV;
    const float denom = NdV * (1.0 - k) + k;

    return nom / denom;
}

float GeomSmith(vec3 N, vec3 V, vec3 L, float roughness){
    const float NdV = max(dot(N, V), 0.0);
    const float NdL = max(dot(N, L), 0.0);
    const float ggx2 = GeomSchlickGGX(NdV, roughness);
    const float ggx1 = GeomSchlickGGX(NdL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0){
    const float x = 1.0 - cosTheta;
    const float x2 = x * x;
    const float x5 = x2 * x2 * x;
    return F0 + (1.0 - F0) * x5;
}

// ------------------------------------------------------------------------

vec3 pbr_lighting(vec3 V, vec3 L, vec3 N, vec3 albedo, vec3 radiance, float roughness, float metalness){
    const float NdL = max(0.0, dot(N, L));
    const vec3 F0 = mix(vec3(0.04), albedo, metalness);
    const vec3 H = normalize(V + L);

    const float NDF = DisGGX(N, H, roughness);
    const float G = GeomSmith(N, V, L, roughness);
    const vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    const vec3 nom = NDF * G * F;
    const float denom = 4.0 * max(dot(N, V), 0.0) * NdL + 0.001;
    const vec3 specular = nom / denom;

    const vec3 kS = F;
    const vec3 kD = (vec3(1.0) - kS) * (1.0 - metalness);

    return (kD * albedo / 3.141592 + specular) * radiance * NdL;
}

vec3 direct_lighting(inout uint s){
    vec4 albedo;
    vec3 N;
    getNormalAndAlbedo(N, albedo);

    const vec3 V = normalize(eye - P);
    const vec3 L = sunDirection;
    const float roughness = roughness_multiplier * (1.0 - albedo.a);
    const float metalness = 0.1;
    const vec3 radiance = sunColor;

    vec3 light = pbr_lighting(V, L, N, albedo.rgb, radiance, roughness, metalness);

    light = light / (light + vec3(1.0));

    return light;
}

vec3 indirect_lighting(inout uint s){
    vec4 albedo;
    vec3 N;
    getNormalAndAlbedo(N, albedo);

    const vec3 mask = albedo.rgb;
    const float roughness = roughness_multiplier * (1.0 - albedo.a);
    const float metalness = 0.99;
    const vec3 I = normalize(P - eye);
    const vec3 R = reflect(I, N);
    const int samples = 16;
    
    vec3 light = vec3(0.0);
    vec3 u, v;
    cosHemiUV(N, u, v);
    for(int i = 0; i < samples; ++i){
        const vec3 randomDir = cosHemi(N, u, v, s);
        const vec3 L = normalize(mix(R, randomDir, roughness));
        const vec3 radiance = texture(env_cm, L).rgb;
        light += pbr_lighting(-I, L, N, mask, radiance, roughness, metalness);
    }

    const float scaling = 1.0 / float(samples);
    light *= scaling * mask;

    light = light / (light + vec3(1.0));

    return light;
}

vec3 visualizeReflections(){
    vec4 albedo;
    vec3 N;
    getNormalAndAlbedo(N, albedo);

    const vec3 mask = albedo.rgb;
    const float roughness = 1.0 - albedo.a;
    const vec3 I = normalize(P - eye);
    const vec3 R = reflect(I, N);
    return texture(env_cm, R).rgb * max(0.0, dot(N, R));
}

vec3 visualizeNormals(){
    vec4 albedo;
    vec3 normal;
    getNormalAndAlbedo(normal, albedo);
    return normal;
}

vec3 visualizeUVs(){
    return vec3(fract(UV.xy), 0.0);
}

vec3 skymap_lighting(){
    vec3 I = normalize(P - eye);
    //return I * 0.5 + 0.5;

    vec3 sky = vec3(0.0);

    vec4 albedo;
    vec3 N;
    getNormalAndAlbedo(N, albedo);

    sky += max(0.0, pow(dot(N, -sunDirection), 200.0)) * sunColor * 1000000.0;

    switch(MID){
        case 0: sky += texture(albedoSampler0, UV).rgb;
        break;
        case 1: sky += texture(albedoSampler1, UV).rgb;
        break;
        case 2: sky += texture(albedoSampler2, UV).rgb;
        break;
        case 3: sky += texture(albedoSampler3, UV).rgb;
        break;
    }
    return sky * 0.5;
}

vec3 visualizeCubemap(){
    vec3 I = normalize(P - eye);
    return texture(env_cm, I).rgb;
}

vec3 visualizeDiffraction(){
    const vec3 I = normalize(P - eye);
    vec4 albedo;
    vec3 N;
    getNormalAndAlbedo(N, albedo);
    const vec3 R = refract(I, N, iorr);
    return texture(env_cm, R).rgb;
}

// ------------------------------------------------------------------------

void main(){
    uint s = uint(seed) 
        ^ uint(gl_FragCoord.x * 10.0) 
        ^ uint(gl_FragCoord.y * 1000.0);

    vec3 lighting;
    if(object_flags == ODF_SKY){
        lighting = skymap_lighting();
    }
    else {
        switch(draw_flags){
            case DF_DIRECT:
                lighting = direct_lighting(s);
                break;
            case DF_INDIRECT:
                lighting = indirect_lighting(s);
                break;
            case DF_NORMALS:
                lighting = visualizeNormals();
                break;
            case DF_REFLECT:
                lighting = visualizeReflections();
                break;
            case DF_UV:
                lighting = visualizeUVs();
                break;
            case DF_DIRECT_CUBEMAP:
                lighting = direct_lighting(s);
                break;
            case DF_VIS_CUBEMAP:
                lighting = visualizeCubemap();
                break;
            case DF_VIS_REFRACT:
                lighting = visualizeDiffraction();
                break;
            default:
                lighting = direct_lighting(s);
                break;
        }
    }

    lighting.rgb.x += 0.0001 * randBi(s);
    lighting.rgb.y += 0.0001 * randBi(s);
    lighting.rgb.z += 0.0001 * randBi(s);

    if(draw_flags != DF_DIRECT_CUBEMAP){
        lighting.rgb = pow(lighting.rgb, vec3(1.0 / 2.2));
    }

    outColor = vec4(lighting.rgb, 1.0);
}