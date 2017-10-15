#version 450 core

out vec4 outColor;

in vec3 MacroNormal;
in vec3 P;
in vec2 UV;

// ------------------------------------------------------------------------

uniform sampler2D albedoSampler;
uniform sampler2D materialSampler;

uniform sampler2D sunDepth;

uniform samplerCube env_cm;

uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 eye;
uniform float sunIntensity;
uniform int seed;
uniform int draw_flags;

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
#define DF_GBUFF            10
#define DF_SKY              11

#define ODF_DEFAULT         0
#define ODF_SKY             1

// -----------------------------------------------------------------------

struct MaterialParams {
    float roughness_offset;
    float roughness_multiplier;
    float metalness_offset;
    float metalness_multiplier;
    float index_of_refraction;
    float bumpiness;
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
    MaterialParams material_params;
};

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

float stratRand(float i, float inv_samples, inout uint s){
    return i * inv_samples + rand(s) * inv_samples;
}

// #9 in http://www-labs.iro.umontreal.ca/~mignotte/IFT2425/Documents/EfficientApproximationArctgFunction.pdf
float fasterAtan(float x){
    return 3.141592 * 0.25 * x 
    - x * (abs(x) - 1.0) 
        * (0.2447 + 0.0663 * abs(x));
}

void findBasis(vec3 N, out vec3 T, out vec3 B){
    if(abs(N.x) > 0.1)
        T = cross(vec3(0.0, 1.0, 0.0), N);
    else
        T = cross(vec3(1.0, 0.0, 0.0), N);
    T = normalize(T);
    B = cross(N, T);
}

// phi: [0, tau]
// theta: [0, 1]
vec3 toCartesian(vec3 T, vec3 B, vec3 N, float phi, float theta){
    const float ts = sqrt(theta);
    return normalize(T * cos(phi) * ts + B * sin(phi) * ts + N * sqrt(1.0 - theta));
}

vec3 cosHemi(vec3 T, vec3 B, vec3 N, vec2 X){
    const float r1 = 3.141592 * 2.0 * X[0];
    const float r2 = X[1];
    return toCartesian(T, B, N, r1, r2);
}

// https://agraphicsguy.wordpress.com/2015/11/01/sampling-microfacet-brdf/
// returns a microfacet normal. reflect across it to get a good light vector
vec3 GGXPDF(const float roughness, const vec2 X, const vec3 T, const vec3 B, const vec3 N){
    const float alpha = roughness; //roughness * roughness;
    const float theta = fasterAtan(alpha * sqrt(X[0] / (1.0 - X[0])));
    const float phi = 2.0 * 3.141592 * X[1];
    return toCartesian(T, B, N, phi, theta);
}

vec3 normalFromHeight(float h){
    const float dhdx = dFdx(h);
    const float dhdy = dFdy(h);

    const vec3 dpdx = dFdx(P);
    const vec3 dpdy = dFdy(P);

    const vec3 r1 = cross(dpdy, MacroNormal);
    const vec3 r2 = cross(MacroNormal, dpdx);

    const vec3 g = (r1 * dhdx + r2 * dhdy) / dot(dpdx, r1);

    return normalize(MacroNormal + g * 0.0015 * material_params.bumpiness);
}

// -------------------------------------------------------------------------------------------

material getMaterial(){
    material mat;

    const vec4 albedo = texture(albedoSampler, UV).rgba;
    const vec4 hmr = texture(materialSampler, UV).rgba;

    mat.normal = normalFromHeight(hmr.x);
    mat.albedo = albedo.rgb;

    mat.metalness = hmr.y * 
        material_params.metalness_multiplier +
        material_params.metalness_offset;
    mat.metalness = clamp(mat.metalness, 0.0, 1.0);

    mat.roughness = hmr.z * 
        material_params.roughness_multiplier + 
        material_params.roughness_offset;
    mat.roughness = clamp(0.22 + 0.78 * mat.roughness, 0.22, 1.0);

    return mat;
}

vec3 environment_cubemap(vec3 dir, float roughness){
    float mip = textureQueryLod(env_cm, dir).x;
    return textureLod(env_cm, dir, mip + roughness * 4.0).rgb;
}

vec3 env_cubemap(vec3 dir){
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
    const vec3 radiance = sunColor * sunIntensity;

    vec3 light = pbr_lighting(V, L, mat, radiance);

    light += vec3(0.01) * mat.albedo;

    return light;
}

vec3 indirect_lighting(inout uint s){
    const material mat = getMaterial();
    const vec3 V = normalize(eye - P);
    vec3 T, B;
    findBasis(mat.normal, T, B);
    
    const float samples = 4.0;
    const float scaling = 1.0 / samples;

    vec3 light = vec3(0.0);
    for(float x = 0.0; x < samples; ++x){
        for(float y = 0.0; y < samples; ++y){
            const vec2 X = vec2(stratRand(x, scaling, s), stratRand(y, scaling, s));
            const vec3 N = GGXPDF(mat.roughness, X, T, B, mat.normal);
            const vec3 L = reflect(-V, N);
            const vec3 radiance = environment_cubemap(L, mat.roughness);
            light += pbr_lighting(V, L, mat, radiance);
        }
    }
    {
        // sometimes the macro normal has the most light. adding a bit of it can greatly reduce noise
        const vec3 R = reflect(-V, mat.normal);
        light += 0.1 * pbr_lighting(V, R, mat, environment_cubemap(R, mat.roughness));
        light += pbr_lighting(V, sunDirection, mat, sunColor * sunIntensity);
    }

    light *= 1.0 / (samples * samples + 1.1);
    light += vec3(0.01) * mat.albedo;

    return light;
}

vec3 visualizeReflections(){
    const material mat = getMaterial();
    const vec3 I = normalize(P - eye);
    const vec3 R = reflect(I, mat.normal);
    return environment_cubemap(R, mat.roughness);
}

vec3 visualizeNormals(){
    const material mat = getMaterial();
    return mat.normal * 0.5 + vec3(0.5);
}

vec3 visualizeUVs(){
    return vec3(fract(UV.xy), 0.0);
}

vec3 visualizeCubemap(){
    vec3 I = normalize(P - eye);
    return env_cubemap(I);
}

vec3 visualizeDiffraction(){
    const vec3 I = normalize(P - eye);
    const material mat = getMaterial();
    const vec3 R = refract(I, mat.normal, material_params.index_of_refraction);
    return env_cubemap(R);
}

vec3 visualizeRoughness(){
    return vec3(getMaterial().roughness);
}

vec3 visualizeMetalness(){
    return vec3(getMaterial().metalness);
}

// ------------------------------------------------------------------------

void main(){
    uint s = uint(seed) 
        ^ uint(gl_FragCoord.x * 39163.0) 
        ^ uint(gl_FragCoord.y * 64601.0);

    vec3 lighting;
    switch(draw_flags)
    {
        default:
        case DF_DIRECT:
        case DF_DIRECT_CUBEMAP:
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

    lighting.rgb.x += 0.0001 * randBi(s);
    lighting.rgb.y += 0.0001 * randBi(s);
    lighting.rgb.z += 0.0001 * randBi(s);

    if(draw_flags != DF_DIRECT_CUBEMAP){
        lighting.rgb = lighting.rgb / (lighting.rgb + vec3(1.0));
        lighting.rgb = pow(lighting.rgb, vec3(1.0 / 2.2));
    }

    outColor = vec4(lighting.rgb, 1.0);
}