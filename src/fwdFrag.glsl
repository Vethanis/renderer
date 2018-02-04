#version 450 core

in vec3 Position;
out vec4 Color;

#define SHARED_UNIFORMS_BINDING 13

struct SharedUniforms
{
    mat4 MVP;
    mat4 IVP;
    mat4 sunMatrix;
    vec4 sunDirection;
    vec4 sunColor; // w -> intensity
    vec4 eye;
    vec4 render_resolution; // zw -> sunNearFar
    vec4 df_translation; // w -> df_pitch;
    vec4 df_scale;
    ivec4 seed_flags; // x -> seed, y -> draw mode, z -> draw pass (shadow, cubemap, color)
    ivec4 sampler_states; // x -> env_cm; y -> sunDepth;
};

layout(std430, binding = SHARED_UNIFORMS_BINDING) buffer SU_BUFFER
{
    SharedUniforms SU;
};

uniform sampler3D distance_field;

float rand( inout uint f) 
{
    f = (f ^ 61) ^ (f >> 16);
    f *= 9;
    f = f ^ (f >> 4);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15);
    return fract(float(f) * 2.3283064e-10);
}

float map(vec3 pt)
{
    pt -= SU.df_translation.xyz;
    pt /= SU.df_scale.xyz;

    return length(pt) - 0.5f;

    //return texture(distance_field, pt).r;
}

vec3 mapN(vec3 pt)
{
    pt -= SU.df_translation.xyz;
    pt /= SU.df_scale.xyz;

    const vec3 e = vec3(0.001, 0.0, 0.0);

    return normalize(vec3(
        map(pt + e.xyz) - map(pt - e.xyz),
        map(pt + e.yxz) - map(pt - e.yxz),
        map(pt + e.yzx) - map(pt - e.yzx)
    ));
}

// ------------------------------------------------------------------------

float DisGGX(vec3 N, vec3 H, float roughness)
{
    const float a = roughness * roughness;
    const float a2 = a * a;
    const float NdH = max(dot(N, H), 0.0);
    const float NdH2 = NdH * NdH;

    const float nom = a2;
    const float denom_term = (NdH2 * (a2 - 1.0) + 1.0);
    const float denom = 3.141592 * denom_term * denom_term;

    return nom / denom;
}

float GeomSchlickGGX(float NdV, float roughness)
{
    const float r = (roughness + 1.0);
    const float k = (r * r) / 8.0;

    const float nom = NdV;
    const float denom = NdV * (1.0 - k) + k;

    return nom / denom;
}

float GeomSmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    const float NdV = max(dot(N, V), 0.0);
    const float NdL = max(dot(N, L), 0.0);
    const float ggx2 = GeomSchlickGGX(NdV, roughness);
    const float ggx1 = GeomSchlickGGX(NdL, roughness);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

vec3 pbr_lighting(vec3 V, vec3 L, vec3 N, vec3 albedo, float roughness, float metalness, vec3 radiance)
{
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

// ------------------------------------------------------------------------

void main()
{
    uint s = uint(SU.seed_flags.x) 
        ^ uint(gl_FragCoord.x * 39163.0) 
        ^ uint(gl_FragCoord.y * 64601.0);

    const vec3 rd = normalize(Position.xyz - SU.eye.xyz);
    const float max_dis = 2.0 * max(SU.df_scale.x, max(SU.df_scale.y, SU.df_scale.z));
    vec3 pt = Position - rd * (max_dis * 0.6);
    
    int i = 0;
    {
        float dis = 10000.0;
        for(; i < 10; ++i)
        {
            dis = map(pt);
            if(abs(dis) < 0.001 || dis > max_dis)
            {
                break;
            }
            pt += rd * dis;
        }

        if(dis > 0.1)
        {
            discard;
        }
    }

    vec4 scrPt = SU.MVP * vec4(pt.xyz, 1.0);
    scrPt /= scrPt.w;
    gl_FragDepth = scrPt.z;

    const vec3 V = -rd;
    const vec3 L = SU.sunDirection.xyz;
    const vec3 N = mapN(pt);
    const vec3 albedo = vec3(0.7, 0.1, 0.2);
    const float roughness = 0.25;
    const float metalness = 0.001;
    const vec3 radiance = SU.sunColor.xyz * SU.sunColor.w;
    vec3 color = pbr_lighting(V, L, N, albedo, roughness, metalness, radiance);

    color = color / (color + vec3(1.0));

    color += 0.01 * vec3(rand(s), rand(s), rand(s));

    Color = vec4(color.xyz, 1.0);
}
