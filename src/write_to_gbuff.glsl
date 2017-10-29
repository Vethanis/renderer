#version 450 core

layout (location = 0) out vec4 gPosition; //   a: roughness
layout (location = 1) out vec4 gNormal;   //   a: metalness
layout (location = 2) out vec4 gAlbedo;   // rgb: albedo, a: depth

// ------------------------------------------------------------------------

in vec3 MacroNormal;
in vec3 P;
in vec2 UV;

// ------------------------------------------------------------------------

uniform sampler2D albedoSampler;
uniform sampler2D materialSampler;

uniform vec3 eye;
uniform int object_flags;
uniform int draw_flags;
uniform int seed;

// -----------------------------------------------------------------------

struct MaterialParams {
    float roughness_offset;
    float roughness_multiplier;
    float metalness_offset;
    float metalness_multiplier;
    float index_of_refraction;
    float bumpiness;
    float heightScale;
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

// -------------------------------------------------------------------------

material getMaterial(vec2 newUv){
    material mat;

    const vec4 albedo = texture(albedoSampler, newUv).rgba;
    const vec4 hmr = texture(materialSampler, newUv).rgba;

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
#define DF_VIS_TANGENTS     12
#define DF_VIS_BITANGENTS   13

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

float stratRand(float i, float inv_samples, inout uint s){
    return i * inv_samples + rand(s) * inv_samples;
}

mat3 GetBasis(vec3 N)
{
    mat3 TBN;
    if(abs(N.x) > 0.001)
        TBN[0] = cross(vec3(0.0, 1.0, 0.0), N);
    else
        TBN[0] = cross(vec3(1.0, 0.0, 0.0), N);
    TBN[0] = normalize(TBN[0]);
    TBN[1] = cross(N, TBN[0]);
    TBN[2] = N;
    return TBN;
}

float getHeight(vec2 uv){
    return texture(materialSampler, uv).x;
}

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir){ 
    const float minLayers = 8;
    const float maxLayers = 32;
    const float heightScale = material_params.heightScale;
    float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));  
    float layerDepth = 1.0 / numLayers;
    float currentLayerDepth = 0.0;
    vec2 P = viewDir.xy / viewDir.z * heightScale; 
    vec2 deltaTexCoords = P / numLayers;
  
    vec2  currentTexCoords     = texCoords;
    float currentDepthMapValue = getHeight(currentTexCoords);
      
    for(int i = 0; i < 16 && currentLayerDepth < currentDepthMapValue; ++i){
        currentTexCoords -= deltaTexCoords;
        currentDepthMapValue = getHeight(currentTexCoords);
        currentLayerDepth += layerDepth;  
    }
    
    vec2 prevTexCoords = currentTexCoords + deltaTexCoords;

    float afterDepth  = currentDepthMapValue - currentLayerDepth;
    float beforeDepth = getHeight(prevTexCoords) - currentLayerDepth + layerDepth;
 
    float weight = afterDepth / (afterDepth - beforeDepth);
    vec2 finalTexCoords = prevTexCoords * weight + currentTexCoords * (1.0 - weight);

    return finalTexCoords;
}

void main(){
    uint s = uint(seed) 
        ^ uint(gl_FragCoord.x * 39163.0) 
        ^ uint(gl_FragCoord.y * 64601.0);

    const vec3 tanV = transpose(GetBasis(MacroNormal)) * normalize(eye - P);

    vec2 newUv = ParallaxMapping(UV, tanV);
    const material mat = getMaterial(newUv);

    gPosition = vec4(P.xyz, mat.roughness);
    gAlbedo = vec4(mat.albedo, getHeight(newUv));
    gNormal = vec4(mat.normal, mat.metalness);

    if(draw_flags == DF_UV)
    {
        gAlbedo.xyz = vec3(fract(newUv).xy, 0.0);
    }
}
