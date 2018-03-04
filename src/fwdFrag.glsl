#version 450 core

out vec4 outColor;

in vec3 P;

// ------------------------------------------------------------------------

uniform sampler2D albedoSampler;
uniform sampler2D materialSampler;

uniform sampler2D sunDepth;

uniform samplerCube env_cm;

uniform mat4 sunMatrix;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 eye;
uniform float sunIntensity;
uniform int seed;

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

float sunShadowing(vec3 p, inout uint s){
    const int samples = 4;
    const float inv_samples = 1.0 / float(samples);
    const float bias = 0.001;

    vec4 projCoords = sunMatrix * vec4(p.xyz, 1.0);
    projCoords /= projCoords.w;
    projCoords = projCoords * 0.5 + 0.5;
    float point_depth = projCoords.z;
    if(point_depth > 1.0)
        return 1.0;

    float occlusion = 0.0;
    const float variance = 0.005;
    for(int i = 0; i < samples; ++i){
        const vec2 p = vec2(randBi(s), randBi(s)) * variance;
        const float d = texture(sunDepth, projCoords.xy + p).r + bias;
        occlusion += point_depth > d ? 0.0 : 1.0;
    }
    occlusion *= inv_samples;

    return occlusion;
}

// phi: [0, tau]
// theta: [0, 1]
vec3 toCartesian(vec3 T, vec3 B, vec3 N, float phi, float theta){
    const float ts = sqrt(theta);
    return normalize(T * cos(phi) * ts + B * sin(phi) * ts + N * sqrt(1.0 - theta));
}

// https://agraphicsguy.wordpress.com/2015/11/01/sampling-microfacet-brdf/
// returns a microfacet normal. reflect across it to get a good light vector
vec3 GGXPDF(const float roughness, const vec2 X, const vec3 T, const vec3 B, const vec3 N){
    const float alpha = roughness; //roughness * roughness;
    const float theta = fasterAtan(alpha * sqrt(X[0] / (1.0 - X[0])));
    const float phi = 2.0 * 3.141592 * X[1];
    return toCartesian(T, B, N, phi, theta);
}

// -------------------------------------------------------------------------------------------

material getMaterial(){
    material mat;

    const vec3 dpdx = dFdx(P);
    const vec3 dpdy = dFdy(P);
    const vec3 MacroNormal = normalize(cross(dpdx, dpdy));

    vec2 uv;
    {
        const vec3 d = abs(MacroNormal);
        const float a = max(max(d.x, d.y), d.z);
        if(a == d.x)
            uv = vec2(P.z, P.y);
        else if(a == d.y)
            uv = vec2(P.x, P.z);
        else
            uv = vec2(P.x, P.y);
    }

    const vec4 albedo = texture(albedoSampler, uv).rgba;
    const vec4 hmr = texture(materialSampler, uv).rgba;

    mat.normal = MacroNormal;
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

    light *= sunShadowing(P, s);

    light += vec3(0.01) * mat.albedo;

    return light;
}

// ------------------------------------------------------------------------

void main()
{
    uint s = uint(seed) 
        ^ uint(gl_FragCoord.x * 39163.0) 
        ^ uint(gl_FragCoord.y * 64601.0);

    vec3 lighting = direct_lighting(s);

    lighting.rgb.x += 0.001 * randBi(s);
    lighting.rgb.y += 0.001 * randBi(s);
    lighting.rgb.z += 0.001 * randBi(s);

    outColor = vec4(lighting.rgb, 1.0);
}