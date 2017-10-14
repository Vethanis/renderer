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

const float depth_scale = 0.005;
const float num_layers = 32.0;
vec2 POM(vec2 uv, vec3 view_dir)
{
    float layer_depth = 1.0 / num_layers;
    float cur_layer_depth = 0.0;
    vec2 delta_uv = view_dir.xy * depth_scale / (view_dir.z * num_layers);
    vec2 cur_uv = uv;

    float depth_from_tex = getHeight(cur_uv);

    for (int i = 0; i < 32; i++) {
        cur_layer_depth += layer_depth;
        cur_uv -= delta_uv;
        depth_from_tex = getHeight(cur_uv);
        if (depth_from_tex < cur_layer_depth) {
            break;
        }
    }

    vec2 prev_uv = cur_uv + delta_uv;
    float next = depth_from_tex - cur_layer_depth;
    float prev = getHeight(prev_uv) - cur_layer_depth + layer_depth;
    float weight = next / (next - prev);
    return mix(cur_uv, prev_uv, weight);
}

float POMOcclusion(vec2 uv, inout uint s){
    const float height = getHeight(uv);
    const float variance = 0.0015;
    const int samples = 8;
    const float inv_samples = 1.0 / float(samples);

    float occlusion = 0.0;
    for(int i = 0; i < samples; ++i){
        const vec2 p = uv + vec2(randBi(s), randBi(s)) * variance;
        const float h = getHeight(p);
        if(h < height){
            occlusion += inv_samples;
        }
    }

    return occlusion;
}

void main(){
    uint s = uint(seed) 
        ^ uint(gl_FragCoord.x * 39163.0) 
        ^ uint(gl_FragCoord.y * 64601.0);

    const vec3 tanV = transpose(GetBasis(MacroNormal)) * normalize(eye - P);

    vec2 newUv = POM(UV, tanV);
    vec3 newP = P + distance(UV, newUv) * normalize(P - eye);
    float occlusion = POMOcclusion(newUv, s);

    const material mat = getMaterial(newUv);

    gPosition = vec4(newP.xyz, mat.roughness);
    gAlbedo = vec4(mat.albedo * (1.0 - occlusion * 0.666), gl_FragCoord.z);
    gNormal = vec4(mat.normal, mat.metalness);

    if(draw_flags == DF_UV)
    {
        gAlbedo.xyz = vec3(fract(newUv).xy, 0.0);
    }
}
