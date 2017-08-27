#version 450 core

out vec4 outColor;

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

uniform samplerCube env_cm;

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 eye;
uniform int seed;
uniform int is_cubemap;
uniform int flags;

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

void getNormalAndAlbedo(out vec3 N, out vec4 albedo){
    switch(MID){
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
}

vec3 direct_lighting(inout uint s){
    vec4 albedo;
    vec3 N;
    getNormalAndAlbedo(N, albedo);

    const vec3 L = sunDirection;
    const float spec = albedo.a * 128.0;
    const vec3 V = normalize(eye - P);
    const vec3 H = normalize(V + L);
    const float D = max(0.0, dot(L, N));
    const float S = D > 0.0 ? pow(max(0.0, dot(H, N)), spec) : 0.0;
    vec3 lighting = (0.01 + D + S) * albedo.rgb;
    return lighting;
}

vec3 direct_lighting_ref(inout uint s){
    vec4 albedo;
    vec3 N;
    getNormalAndAlbedo(N, albedo);

    const vec3 L = sunDirection;
    const float spec = albedo.a * 128.0;
    const vec3 V = normalize(eye - P);
    const vec3 H = normalize(V + L);
    const float D = max(0.0, dot(L, N));
    const float S = D > 0.0 ? pow(max(0.0, dot(H, N)), spec) : 0.0;
    const vec3 env = S * texture(env_cm, N).rgb;
    vec3 lighting = (vec3(0.01 + D) + env) * albedo.rgb;
    return lighting;
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

vec3 indirect_lighting(inout uint s){
    vec4 albedo;
    vec3 N;
    getNormalAndAlbedo(N, albedo);

    const vec3 mask = albedo.rgb;
    const float roughness = 1.0 - albedo.a;
    const vec3 V = normalize(P - eye);
    const vec3 R = normalize(reflect(V, N));
    const int samples = 16;
    const float scaling = 1.0 / float(samples);
    
    vec3 light = vec3(0.0);
    vec3 u, v;
    cosHemiUV(N, u, v);
    for(int i = 0; i < samples; ++i){
        const vec3 randomDir = cosHemi(N, u, v, s);
        const vec3 dir = normalize(mix(R, randomDir, roughness));
        const float dirscale = max(0.0, dot(N, dir));
        light += mask * texture(env_cm, dir).rgb * scaling * dirscale;
    }

    return light;
}

vec3 skymap_lighting(){
    const float sunMag = pow(max(0.0, dot(TBN[2], -sunDirection)), 256.0) * 10000000.0;
    const vec3 sun = sunColor * sunMag;
    switch(MID){
        case 0: return sun + texture(albedoSampler0, UV).rgb * 0.01;
        case 1: return sun + texture(albedoSampler1, UV).rgb* 0.01;
        case 2: return sun + texture(albedoSampler2, UV).rgb* 0.01;
        case 3: return sun + texture(albedoSampler3, UV).rgb* 0.01;
    }
    return sun;
}

void main(){
    uint s = uint(seed) ^ uint(UV.x * 951489.0) ^ uint(UV.y * 7561182.0);

    vec3 lighting;
    if((flags & 1) == 1){
        lighting = skymap_lighting();
    }
    else if(is_cubemap == 1){
        lighting = direct_lighting(s);
    }
    else{
        lighting = direct_lighting_ref(s);
    }

    lighting.rgb.x += 0.0001 * randBi(s);
    lighting.rgb.y += 0.0001 * randBi(s);
    lighting.rgb.z += 0.0001 * randBi(s);

    outColor = vec4(pow(lighting.rgb, vec3(0.5)), 1.0);

}