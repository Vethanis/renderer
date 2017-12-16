#version 450 core

out vec4 outColor;

in vec3 MacroNormal;
in vec3 P;
in vec3 Color;
in vec3 Material; // roughness, metalness, ao

#define getAlbedo() Color.xyz
#define getPosition() P.xyz
#define getNormal() MacroNormal.xyz
#define getRoughness() Material.x
#define getMetalness() Material.y
#define getAO() Material.z

// ------------------------------------------------------------------------

uniform sampler2D sunDepth;

uniform samplerCube env_cm;

uniform mat4 sunMatrix;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 eye;
uniform float sunIntensity;
uniform int seed;

// ------------------------------------------------------------------------

float rand( inout uint f) 
{
    f = (f ^ 61) ^ (f >> 16);
    f *= 9;
    f = f ^ (f >> 4);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15);
    return fract(float(f) * 2.3283064e-10);
}

float randBi(inout uint s)
{
    return rand(s) * 2.0 - 1.0;
}

// #9 in http://www-labs.iro.umontreal.ca/~mignotte/IFT2425/Documents/EfficientApproximationArctgFunction.pdf
float fasterAtan(float x)
{
    return 3.141592 * 0.25 * x 
    - x * (abs(x) - 1.0) 
        * (0.2447 + 0.0663 * abs(x));
}

float sunShadowing(vec3 p, inout uint s)
{
    const int samples = 8;
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
    for(int i = 0; i < samples; ++i)
    {
        const vec2 p = vec2(randBi(s), randBi(s)) * variance;
        const float d = texture(sunDepth, projCoords.xy + p).r + bias;
        occlusion += point_depth > d ? 0.0 : 1.0;
    }
    occlusion *= inv_samples;

    return occlusion;
}

// ------------------------------------------------------------------------

float DisGGX(vec3 N, vec3 H)
{
    const float a = getRoughness() * getRoughness();
    const float a2 = a * a;
    const float NdH = max(dot(N, H), 0.0);
    const float NdH2 = NdH * NdH;

    const float nom = a2;
    const float denom_term = (NdH2 * (a2 - 1.0) + 1.0);
    const float denom = 3.141592 * denom_term * denom_term;

    return nom / denom;
}

float GeomSchlickGGX(float NdV)
{
    const float r = (getRoughness() + 1.0);
    const float k = (r * r) / 8.0;

    const float nom = NdV;
    const float denom = NdV * (1.0 - k) + k;

    return nom / denom;
}

float GeomSmith(vec3 N, vec3 V, vec3 L)
{
    const float NdV = max(dot(N, V), 0.0);
    const float NdL = max(dot(N, L), 0.0);
    const float ggx2 = GeomSchlickGGX(NdV);
    const float ggx1 = GeomSchlickGGX(NdL);

    return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

// ------------------------------------------------------------------------

vec3 pbr_lighting(vec3 V, vec3 L, vec3 radiance)
{
    const float NdL = max(0.0, dot(getNormal(), L));
    const vec3 F0 = mix(vec3(0.04), getAlbedo(), getMetalness());
    const vec3 H = normalize(V + L);

    const float NDF = DisGGX(getNormal(), H);
    const float G = GeomSmith(getNormal(), V, L);
    const vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    const vec3 nom = NDF * G * F;
    const float denom = 4.0 * max(dot(getNormal(), V), 0.0) * NdL + 0.001;
    const vec3 specular = nom / denom;

    const vec3 kS = F;
    const vec3 kD = (vec3(1.0) - kS) * (1.0 - getMetalness());

    return (kD * Color / 3.141592 + specular) * radiance * NdL;
}

vec3 direct_lighting(inout uint s)
{
    const vec3 V = normalize(eye - P);
    const vec3 L = sunDirection;
    const vec3 radiance = sunColor * sunIntensity;

    vec3 light = pbr_lighting(V, L, radiance);

    light *= 1.0 - getAO();
    light *= sunShadowing(P, s);

    light += vec3(0.01) * getAlbedo();

    return light;
}

// ------------------------------------------------------------------------

void main()
{
    uint s = uint(seed) 
        ^ uint(gl_FragCoord.x * 39163.0) 
        ^ uint(gl_FragCoord.y * 64601.0);

    vec3 lighting = direct_lighting(s);

    lighting.rgb.x += 0.0001 * randBi(s);
    lighting.rgb.y += 0.0001 * randBi(s);
    lighting.rgb.z += 0.0001 * randBi(s);

    outColor = vec4(lighting.rgb, 1.0);
}