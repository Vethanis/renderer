#version 450 core

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
#define DF_VIS_SUN_SHADOW_DEPTH 14
#define DF_VIS_VELOCITY     15
#define DF_VIS_SHADOW_BUFFER 16
#define DF_VIS_AMBIENT_OCCLUSION 17

#define ODF_DEFAULT         0
#define ODF_SKY             1

#define TX_ALBEDO_CHANNEL   0
#define TX_MATERIAL_CHANNEL 1
#define TX_SUN_CHANNEL      2
#define TX_CUBEMAP_CHANNEL  3
#define TX_POSITION_CHANNEL 4
#define TX_NORMAL_CHANNEL   5

// ------------------------------------------------------------------------

layout (location = 0) out vec4 outColor;
in vec2 fragUv;

// ------------------------- samplers ---------------------------------

uniform sampler2D positionSampler;
uniform sampler2D normalSampler;
uniform sampler2D albedoSampler;
uniform sampler2D velocitySampler;
uniform sampler2D prevColor;
uniform samplerCube env_cm;
uniform sampler2D sunDepth;

uniform mat4 MVP;
uniform mat4 IVP;
uniform mat4 prevVP;
uniform mat4 sunMatrix;
uniform vec3 sunDirection;
uniform vec3 sunColor;
uniform vec3 eye;
uniform vec2 render_resolution;
uniform vec2 sunNearFar;
uniform float sunIntensity;
uniform int seed;
uniform int draw_flags;

struct material
{
    vec4 position;
    vec4 normal;
    vec4 albedo;
};

#define mat_metalness(x) x.normal.w
#define mat_roughness(x) x.position.w
#define mat_depth(x) x.albedo.w
#define mat_position(x) x.position.xyz
#define mat_normal(x) x.normal.xyz
#define mat_albedo(x) x.albedo.xyz

material getMaterial(){
    return material(
        texture(positionSampler, fragUv),
        texture(normalSampler, fragUv),
        texture(albedoSampler, fragUv)
    );
}

vec3 environment_cubemap(vec3 dir, float roughness){
    float mip = textureQueryLod(env_cm, dir).x;
    return textureLod(env_cm, dir, mip + roughness * 10.0).rgb;
}

vec3 env_cubemap(vec3 dir){
    return texture(env_cm, dir).rgb;
}

float getHeight(vec2 uv){
    return texture(albedoSampler, uv).w;
}

float getDepth(vec2 uv){
    return texture(velocitySampler, uv).w;
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

float stratRand(float i, float inv_samples, inout uint s){
    return i * inv_samples + rand(s) * inv_samples;
}

float stratRandBi(float i, float inv_samples, inout uint s){
    return 2.0 * (inv_samples + rand(s) * inv_samples) - 1.0;
}

void findBasis(vec3 N, out vec3 T, out vec3 B){
    if(abs(N.x) > 0.001)
        T = cross(vec3(0.0, 1.0, 0.0), N);
    else
        T = cross(vec3(1.0, 0.0, 0.0), N);
    T = normalize(T);
    B = cross(N, T);
}

// -------------------------------------------------------------------------------------------

float sunShadowing(vec3 p, inout uint s){
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
    for(int i = 0; i < samples; ++i){
        const vec2 p = vec2(randBi(s), randBi(s)) * variance;
        const float d = texture(sunDepth, projCoords.xy + p).r + bias;
        occlusion += point_depth > d ? 0.0 : 1.0;
    }
    occlusion *= inv_samples;

    return occlusion;
}

vec3 toWorld(float x, float y, float z){
    vec4 t = vec4(x, y, z, 1.0);
    t = IVP * t;
    return vec3(t/t.w);
}

float HeightOcclusion(vec3 P, vec3 N, inout uint s){
    const int samples = 4;
    const float base = getHeight(fragUv);
    if(base == 0.0)
        return 0.0;
        
    vec3 T, B;
    findBasis(N, T, B);

    float occlusion = 0.0;
    float radius = 0.0025;
    for(int i = 0; i < samples; ++i)
    {
        const vec3 pt = P + randBi(s) * radius * T + randBi(s) * radius * B;
        vec4 pt4 = MVP * vec4(pt.xyz, 1.0);
        pt4 /= pt4.w;
        pt4 = pt4 * 0.5 + 0.5;
        const float p = getHeight(pt4.xy);
        occlusion += p < base ? 1.0 : 0.0;
        radius *= 2.0;
    }
    return occlusion / float(samples);
}

float AmbientOcclusion(vec3 P, vec3 N, inout uint s)
{
    const int samples = 4;
    float occlusion = 0.0;
    float radius = 0.05;
    for(int i = 0; i < samples; ++i)
    {
        vec4 screenPoint = MVP * vec4(P + radius * (N + vec3(rand(s), rand(s), rand(s)) * 2.0 - 1.0), 1.0);
        screenPoint /= screenPoint.w;
        screenPoint.xyz = screenPoint.xyz * 0.5 + 0.5;
        const float depth = getDepth(screenPoint.xy);

        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(screenPoint.z - depth));

        occlusion += depth < screenPoint.z ? rangeCheck : 0.0;
        radius *= 3.3333;
    }

    return occlusion / float(samples);
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

    const float NdL = max(0.0, dot(mat_normal(mat), L));
    const vec3 F0 = mix(vec3(0.04), mat_albedo(mat), mat_metalness(mat));
    const vec3 H = normalize(V + L);

    const float NDF = DisGGX(mat_normal(mat), H, mat_roughness(mat));
    const float G = GeomSmith(mat_normal(mat), V, L, mat_roughness(mat));
    const vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    const vec3 nom = NDF * G * F;
    const float denom = 4.0 * max(dot(mat_normal(mat), V), 0.0) * NdL + 0.001;
    const vec3 specular = nom / denom;

    const vec3 kS = F;
    const vec3 kD = (vec3(1.0) - kS) * (1.0 - mat_metalness(mat));

    return (kD * mat_albedo(mat) / 3.141592 + specular) * radiance * NdL;
}

vec3 direct_lighting(inout uint s){
    const material mat = getMaterial();
    const vec3 V = normalize(eye - mat_position(mat));
    const vec3 L = sunDirection;
    const vec3 radiance = sunColor * sunIntensity;

    vec3 light = sunShadowing(mat_position(mat), s) * pbr_lighting(V, L, mat, radiance);

    light += vec3(0.01) * mat_albedo(mat);

    return light;
}

vec3 indirect_lighting(inout uint s){
    const material mat = getMaterial();
    const vec3 V = normalize(eye - mat_position(mat));
    vec3 T, B;
    findBasis(mat_normal(mat), T, B);

    vec3 light = vec3(0.0);
    {
        const vec3 R = reflect(-V, mat_normal(mat));
        light += pbr_lighting(V, R, mat, environment_cubemap(R, mat_roughness(mat)));
    }

    for(int i = 0; i < 4; ++i){
        vec3 r = vec3(randBi(s), randBi(s), randBi(s));
        if(dot(r, mat_normal(mat)) < 0.001){
            r = -r;
        }
        r = normalize(r);
        light += pbr_lighting(V, r, mat, environment_cubemap(r, mat_roughness(mat)));
    }

    light *= 3.141592 / (6.0);
    light *= 1.0 + (2.0 * mat_roughness(mat) - 1.0); // hacky, but rough materials require more samples to get same luminosity, so make them brighter
    
    light += sunShadowing(mat_position(mat), s) * pbr_lighting(V, sunDirection, mat, sunColor * sunIntensity);

    light *= (1.0 - HeightOcclusion(mat_position(mat), mat_normal(mat), s));
    light *= (1.0 - AmbientOcclusion(mat_position(mat), mat_normal(mat), s));

    return light;
}

vec3 visualizeReflections(){
    const material mat = getMaterial();
    const vec3 I = normalize(mat_position(mat) - eye);
    const vec3 R = reflect(I, mat_normal(mat));
    return environment_cubemap(R, mat_roughness(mat));
}

vec3 visualizeNormals(){
    const material mat = getMaterial();
    return mat_normal(mat);
}

vec3 visualizeTangents(){
    const material mat = getMaterial();
    vec3 T, B;
    findBasis(mat_normal(mat), T, B);
    return T;
}

vec3 visualizeBitangents(){
    const material mat = getMaterial();
    vec3 T, B;
    findBasis(mat_normal(mat), T, B);
    return B;
}

vec3 visualizeUVs(){
    return vec3(mat_albedo(getMaterial()).xy, 0.0);
}

vec3 visualizeCubemap(){
    const material mat = getMaterial();
    vec3 I = normalize(mat_position(mat) - eye);
    return env_cubemap(I);
}

vec3 visualizeDiffraction(){
    const material mat = getMaterial();
    const vec3 I = normalize(mat_position(mat) - eye);
    const vec3 R = refract(I, mat_normal(mat), 1.000293 / 1.33);
    return env_cubemap(R);
}

vec3 visualizeRoughness(){
    const material mat = getMaterial();
    return vec3(mat_roughness(mat));
}

vec3 visualizeMetalness(){
    const material mat = getMaterial();
    return vec3(mat_metalness(mat));
}

vec3 visualizeShadow(){
    const material mat = getMaterial();
    vec3 p = mat_position(mat);
    vec4 projCoords = sunMatrix * vec4(p.xyz, 1.0);
    projCoords /= projCoords.w;
    projCoords = projCoords * 0.5 + 0.5;
    float light_depth = texture(sunDepth, projCoords.xy).r;
    return vec3(light_depth);
}

vec2 rsi(vec3 r0, vec3 rd, float sr) {
    float a = dot(rd, rd);
    float b = 2.0 * dot(rd, r0);
    float c = dot(r0, r0) - (sr * sr);
    float d = (b*b) - 4.0*a*c;
    if (d < 0.0) return vec2(1e5,-1e5);
    return vec2(
        (-b - sqrt(d))/(2.0*a),
        (-b + sqrt(d))/(2.0*a)
    );
}

// https://github.com/wwwtyro/glsl-atmosphere
vec3 atmosphere(vec3 r, float iSun, float rPlanet, float rAtmos, vec3 kRlh, float kMie, float shRlh, float shMie, float g) {
    const int iSteps = 16;
    const int jSteps = 16;
    const float PI = 3.141592;

    const vec3 pSun = sunDirection;
    const vec3 r0 = vec3(0.0,6372e3,0.0);

    vec2 p = rsi(r0, r, rAtmos);
    if (p.x > p.y) return vec3(0,0,0);
    p.y = min(p.y, rsi(r0, r, rPlanet).x);
    float iStepSize = (p.y - p.x) / float(iSteps);

    float mu = dot(r, pSun);
    float mumu = mu * mu;
    float gg = g * g;
    float pRlh = 3.0 / (16.0 * PI) * (1.0 + mumu);
    float pMie = 3.0 / (8.0 * PI) * ((1.0 - gg) * (mumu + 1.0)) / (pow(1.0 + gg - 2.0 * mu * g, 1.5) * (2.0 + gg));

    vec3 totalRlh = vec3(0.0);
    vec3 totalMie = vec3(0.0);
    float iOdRlh = 0.0;
    float iOdMie = 0.0;
    float iTime = 0.0;
    for (int i = 0; i < iSteps; i++) {
        vec3 iPos = r0 + r * (iTime + iStepSize * 0.5);
        float iHeight = length(iPos) - rPlanet;
        float odStepRlh = exp(-iHeight / shRlh) * iStepSize;
        float odStepMie = exp(-iHeight / shMie) * iStepSize;

        iOdRlh += odStepRlh;
        iOdMie += odStepMie;

        float jStepSize = rsi(iPos, pSun, rAtmos).y / float(jSteps);
        float jTime = 0.0;
        float jOdRlh = 0.0;
        float jOdMie = 0.0;

        for (int j = 0; j < jSteps; j++) {
            vec3 jPos = iPos + pSun * (jTime + jStepSize * 0.5);
            float jHeight = length(jPos) - rPlanet;

            jOdRlh += exp(-jHeight / shRlh) * jStepSize;
            jOdMie += exp(-jHeight / shMie) * jStepSize;

            jTime += jStepSize;
        }

        vec3 attn = exp(-(kMie * (iOdMie + jOdMie) + kRlh * (iOdRlh + jOdRlh)));

        totalRlh += odStepRlh * attn;
        totalMie += odStepMie * attn;

        iTime += iStepSize;
    }

    return iSun * (pRlh * kRlh * totalRlh + pMie * kMie * totalMie);
}

vec3 skylight(){
    const vec2 uv = (gl_FragCoord.xy  / render_resolution) * 2.0 - vec2(1.0);
    const vec3 rd = normalize(toWorld(uv.x, uv.y, 0.0) - eye);
    return atmosphere(rd, sunIntensity, 6371e3, 6471e3, 
        vec3(5.5e-6, 13.0e-6, 22.4e-6), 
        21e-6, 8e3, 1.2e3, 0.758);
}

vec3 visualizeShadowBuffer()
{
    float near = sunNearFar.x;
    float far = sunNearFar.y;
    float depth = texture(sunDepth, fragUv.xy).r;
    depth = (2.0 * near) / (far + near - depth * (far - near));
    return vec3(depth);
}


vec3 visualizeAmbientOcclusion(inout uint s)
{
    const material mat = getMaterial();
    float ao = AmbientOcclusion(mat_position(mat), mat_normal(mat), s);
    //ao += HeightOcclusion(mat_position(mat), mat_normal(mat), s);
    return vec3(1.0 - ao);
}

// -----------------------------------------------------------------------

void main(){
    uint s = uint(seed) 
        ^ uint(gl_FragCoord.x * 39163.0) 
        ^ uint(gl_FragCoord.y * 64601.0);

    switch(draw_flags)
    {
        case DF_VIS_SHADOW_BUFFER:
            outColor.xyz = visualizeShadowBuffer().xyz;
            return;
    }

    const vec3 albedo = texture(albedoSampler, fragUv).rgb;
    const bool shouldSkylight = albedo.x == 0.0 && albedo.y == 0.0 && albedo.z == 0.0;
    
    vec3 lighting;
    if(shouldSkylight)
    {
        lighting = skylight();
    }
    else
    {
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
            case DF_VIS_TANGENTS:
                lighting = visualizeTangents();
                break;
            case DF_VIS_BITANGENTS:
                lighting = visualizeBitangents();
                break;
            case DF_VIS_SUN_SHADOW_DEPTH:
                lighting = visualizeShadow();
                break;
            case DF_VIS_VELOCITY:
                outColor.xyz = texture(velocitySampler, fragUv).xyz;
                return;
            case DF_VIS_AMBIENT_OCCLUSION:
                outColor.xyz = visualizeAmbientOcclusion(s);
                return;
        }
    }

    lighting.rgb = lighting.rgb / (lighting.rgb + vec3(1.0));

    if(!shouldSkylight)
    {
        vec4 P = texture(positionSampler, fragUv);
        const vec3 velocity = texture(velocitySampler, fragUv).xyz;
        P.xyz -= velocity;
        P.w = 1.0;
        P = prevVP * P;
        P /= P.w;
        P.xy = P.xy * 0.5 + 0.5;

        if(P.x <= 1.0 && P.x >= 0.0)
        {
            if(P.y <= 1.0 && P.y >= 0.0)
            {
                vec3 prev = texture(prevColor, P.xy).rgb;
                if(prev.x + prev.y + prev.z > 0.00001)
                    lighting.rgb = mix(lighting.rgb, prev, 0.5);
            }
        }
    }

    outColor = vec4(lighting.rgb, 1.0);
}