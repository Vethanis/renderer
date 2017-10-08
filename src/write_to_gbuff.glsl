#version 450 core

layout (location = 0) out vec4 gPosition; //   a: roughness
layout (location = 1) out vec4 gNormal;   //   a: metalness
layout (location = 2) out vec4 gAlbedo;   // rgb: albedo, a: index of refraction

// ------------------------------------------------------------------------

in vec3 MacroNormal;
in vec3 P;
in vec2 UV;

// ------------------------------------------------------------------------

uniform sampler2D albedoSampler;
uniform sampler2D materialSampler;

uniform int object_flags;

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
    mat.roughness = clamp(mat.roughness, 0.22, 1.0);

    return mat;
}

// ------------------------------------------------------------------------

#define ODF_DEFAULT         0
#define ODF_SKY             1

void main(){
    const material mat = getMaterial();
    gPosition = vec4(P.xyz, mat.roughness);
    gAlbedo = vec4(mat.albedo, material_params.index_of_refraction);
    gNormal = vec4(mat.normal, mat.metalness);
}
