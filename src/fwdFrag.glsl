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

// ------------------------------------------------------------------------

#define PREPASS_ENABLED     1

#define DF_DIRECT           0
#define DF_INDIRECT         1
#define DF_NORMALS          2
#define DF_REFLECT          3
#define DF_UV               4
#define DF_DIRECT_CUBEMAP   5
#define DF_VIS_CUBEMAP      6
#define DF_VIS_REFRACT      7
#define DF_VIS_ROUGHNESS    8
#define DF_VIS_METALNESS    9

#define ODF_DEFAULT         0
#define ODF_SKY             1

// -----------------------------------------------------------------------

struct MaterialParams {
    float roughness_offset;
    float roughness_multiplier;
    float metalness_offset;
    float metalness_multiplier;
    float index_of_refraction;
    float _pad1;
    float _pad2;
    float _pad3;
};

struct material {
    vec3 albedo;
    float roughness;
    vec3 normal;
    float metalness;
};

// ------------------------------------------------------------------------

layout(std140) uniform materialparams_ubo
{
    MaterialParams material_params[4];
};

// ------------------------------------------------------------------------

// COURTESY OF: Leonard Ritter (@paniq)
// Filmic Reinhard, a simpler tonemapping 
// operator with a single coefficient
// regulating the toe size.

// The operator ensures that f(0.5) = 0.5
float filmic_reinhard_curve (float x) {
    // T = 0: no toe, classic Reinhard
    const float T = 0.01;
    const float q = (T + 1.0)*x*x;    
	return q / (q + x + T);
}

float inverse_filmic_reinhard_curve (float x) {
    // T = 0: no toe, classic Reinhard
    const float T = 0.01;
    const float q = -2.0 * (T + 1.0) * (x - 1.0);
    return (x + sqrt(x*(x + 2.0*T*q))) / q;
}

vec3 filmic_reinhard(vec3 x) {
    // linear white point
    const float W = 11.2;
    const float w = filmic_reinhard_curve(W);
    return vec3(
        filmic_reinhard_curve(x.r),
        filmic_reinhard_curve(x.g),
        filmic_reinhard_curve(x.b)) / w;
}

vec3 inverse_filmic_reinhard(vec3 x) {
    // linear white point
    const float W = 11.2;
    x *= filmic_reinhard_curve(W);
    return vec3(
        inverse_filmic_reinhard_curve(x.r),
        inverse_filmic_reinhard_curve(x.g),
        inverse_filmic_reinhard_curve(x.b));
}

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

material getMaterial(){
    material mat;
    vec4 albedo, normal;

    switch(MID){
        default:
        case 0:
        {
            albedo = texture(albedoSampler0, UV).rgba;
            normal = texture(normalSampler0, UV).rgba;
        }
        break;
        case 1:
        {
            albedo = texture(albedoSampler1, UV).rgba;
            normal = texture(normalSampler1, UV).rgba;
        }
        break;
        case 2:
        {
            albedo = texture(albedoSampler2, UV).rgba;
            normal = texture(normalSampler2, UV).rgba;
        }
        break;
        case 3:
        {
            albedo = texture(albedoSampler3, UV).rgba;
            normal = texture(normalSampler3, UV).rgba;
        }
        break;
    }
    
    const float roughness_offset = material_params[MID].roughness_offset;
    const float roughness_multiplier = material_params[MID].roughness_multiplier;
    const float metalness_offset = material_params[MID].metalness_offset;
    const float metalness_multiplier = material_params[MID].metalness_multiplier;

    mat.albedo = pow(albedo.rgb, vec3(2.2));
    mat.roughness = clamp(albedo.a * roughness_multiplier + roughness_offset, 0.01, 0.99);
    mat.normal = normalize(TBN * normalize(normal.xyz * 2.0 - 1.0));
    mat.metalness = clamp(normal.a * metalness_multiplier + metalness_offset, 0.01, 0.99);

    return mat;
}

vec3 environment_cubemap(vec3 dir){
    return texture(env_cm, dir).rgb;
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
    const float k = (r * r) / 8.0;

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
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// ------------------------------------------------------------------------

vec3 pbr_lighting(vec3 V, vec3 L, const material mat, vec3 radiance){
    const float NdL = max(0.0, dot(mat.normal, L));
    const vec3 F0 = mix(vec3(0.04), mat.albedo, mat.metalness);
    const vec3 H = normalize(V + L);

    const float NDF = DisGGX(mat.normal, H, mat.roughness);
    const float G = GeomSmith(mat.normal, V, L, mat.roughness);
    const vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    const vec3 nom = NDF * G * F;
    const float denom = 4.0 * max(dot(mat.normal, V), 0.0) * NdL + 0.001;
    const vec3 specular = nom / denom;

    const vec3 kS = F;
    const vec3 kD = (vec3(1.0) - kS) * (1.0 - mat.metalness);

    return (kD * mat.albedo / 3.141592 + specular) * radiance * NdL;
}

vec3 direct_lighting(inout uint s){
    const material mat = getMaterial();

    const vec3 V = normalize(eye - P);
    const vec3 L = sunDirection;
    const vec3 radiance = sunColor;

    vec3 light = pbr_lighting(V, L, mat, radiance);

    light += vec3(0.01) * mat.albedo;

    return light;
}

vec3 indirect_lighting(inout uint s){
    const material mat = getMaterial();
    const vec3 V = normalize(eye - P);
    const vec3 R = reflect(-V, mat.normal);
    const int samples = 64;
    const float scaling = 1.0 / float(samples);
    
    vec3 light = vec3(0.0);
    vec3 u, v;
    cosHemiUV(mat.normal, u, v);
    for(int i = 0; i < samples; ++i){
        const vec3 L = cosHemi(mat.normal, u, v, s);
        const vec3 radiance = environment_cubemap(L);
        light += pbr_lighting(V, L, mat, radiance);
    }

    light *= scaling;

    return light;
}

vec3 visualizeReflections(){
    const material mat = getMaterial();
    const vec3 I = normalize(P - eye);
    const vec3 R = reflect(I, mat.normal);
    return environment_cubemap(R) * max(0.0, dot(mat.normal, R));
}

vec3 visualizeNormals(){
    const material mat = getMaterial();
    return mat.normal;
}

vec3 visualizeUVs(){
    return vec3(fract(UV.xy), 0.0);
}

vec3 skymap_lighting(){
    vec3 sky = vec3(0.0);
    const material mat = getMaterial();
    sky += max(0.0, pow(dot(mat.normal, -sunDirection), 200.0)) * sunColor * 1000000.0;
    sky += mat.albedo * 0.5;
    return sky;
}

vec3 visualizeCubemap(){
    vec3 I = normalize(P - eye);
    return environment_cubemap(I);
}

vec3 visualizeDiffraction(){
    const vec3 I = normalize(P - eye);
    const material mat = getMaterial();
    const vec3 R = refract(I, mat.normal, material_params[MID].index_of_refraction);
    return environment_cubemap(R);
}

vec3 visualizeRoughness(){
    const material mat = getMaterial();
    return vec3(mat.roughness);
}

vec3 visualizeMetalness(){
    const material mat = getMaterial();
    return vec3(mat.metalness);
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
            default:
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
            case DF_VIS_ROUGHNESS:
                lighting = visualizeRoughness();
                break;
            case DF_VIS_METALNESS:
                lighting = visualizeMetalness();
                break;
        }
    }

    lighting.rgb.x += 0.0001 * randBi(s);
    lighting.rgb.y += 0.0001 * randBi(s);
    lighting.rgb.z += 0.0001 * randBi(s);

    if(draw_flags != DF_DIRECT_CUBEMAP){
        lighting.rgb = filmic_reinhard(lighting.rgb);
        lighting.rgb = pow(lighting.rgb, vec3(1.0 / 2.2));
    }

    outColor = vec4(lighting.rgb, 1.0);
}