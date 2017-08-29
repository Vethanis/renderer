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
uniform int draw_flags;
uniform int object_flags;

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
    const float spec = albedo.a * 256.0;
    const vec3 V = normalize(eye - P);
    const vec3 H = normalize(V + L);
    const float D = max(0.0, dot(L, N));
    const float S = D > 0.0 ? pow(max(0.0, dot(H, N)), spec) : 0.0;
    const vec3 env = S * sunColor;
    vec3 lighting = (vec3(0.01 + D) + env) * albedo.rgb;
    return lighting;
}

vec3 direct_lighting_ref(inout uint s){
    vec4 albedo;
    vec3 N;
    getNormalAndAlbedo(N, albedo);

    const vec3 L = sunDirection;
    const float spec = albedo.a * 256.0;
    const vec3 V = normalize(eye - P);
    const vec3 H = normalize(V + L);
    const float D = max(0.0, dot(L, N));
    const float S = D > 0.0 ? pow(max(0.0, dot(H, N)), spec) : 0.0;
    const vec3 R = normalize(reflect(-V, N));
    const vec3 env = S * texture(env_cm, R).rgb;
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
    const vec3 I = normalize(P - eye);
    const vec3 R = reflect(I, N);
    const int samples = 16;
    
    vec3 light = vec3(0.0);
    vec3 u, v;
    cosHemiUV(N, u, v);
    for(int i = 0; i < samples; ++i){
        const vec3 randomDir = cosHemi(N, u, v, s);
        const vec3 L = normalize(mix(R, randomDir, roughness));
        light += texture(env_cm, L).rgb * max(0.0, dot(N, L));
    }

    const float scaling = 1.0 / float(samples);
    light *= scaling * mask;

    return light;
}

vec3 visualizeReflections(inout uint s){
    vec4 albedo;
    vec3 N;
    getNormalAndAlbedo(N, albedo);

    const vec3 mask = albedo.rgb;
    const float roughness = 1.0 - albedo.a;
    const vec3 I = normalize(P - eye);
    const vec3 R = reflect(I, N);
    return texture(env_cm, R).rgb;
}

vec3 skymap_lighting(){
    vec3 sky = vec3(0.0);

    vec4 albedo;
    vec3 N;
    getNormalAndAlbedo(N, albedo);

    sky += max(0.0, pow(dot(-N, sunDirection), 128.0)) * sunColor * 10000.0;

    switch(MID){
        case 0: sky += texture(albedoSampler0, UV).rgb;
        break;
        case 1: sky += texture(albedoSampler1, UV).rgb;
        break;
        case 2: sky += texture(albedoSampler2, UV).rgb;
        break;
        case 3: sky += texture(albedoSampler3, UV).rgb;
        break;
    }
    return sky * 0.01;
}

vec3 visualizeCubemap(){
    vec3 I = normalize(P - eye);
    return texture(env_cm, I).rgb;
}

void main(){
    uint s = uint(seed) 
        ^ uint(gl_FragCoord.x * 10.0) 
        ^ uint(gl_FragCoord.y * 1000.0);

    vec3 lighting;
    if((object_flags & 1) == 1){
        lighting = skymap_lighting();
    }
    else if((draw_flags & 1) == 1){
        lighting = direct_lighting(s);
    }
    else if((draw_flags & 2) == 2){
        lighting = direct_lighting_ref(s);
    }
    else if((draw_flags & 4) == 4){
        lighting = indirect_lighting(s);
    }
    else if((draw_flags & 8) == 8){
        vec4 albedo;
        vec3 normal;
        getNormalAndAlbedo(normal, albedo);
        outColor = vec4(normal.rgb * 0.5 + 0.5, 1.0);
        return;
    }
    else if((draw_flags & 16) == 16){
        outColor = vec4(visualizeReflections(s), 1.0);
        return;
    }
    else if((draw_flags & 32) == 32){
        outColor = vec4(fract(UV.xy), 0.0, 1.0);
        return;
    }
    else if((draw_flags & 128) == 128){
        outColor = vec4(visualizeCubemap().rgb, 1.0);
        return;
    }

    lighting.rgb.x += 0.0001 * randBi(s);
    lighting.rgb.y += 0.0001 * randBi(s);
    lighting.rgb.z += 0.0001 * randBi(s);

    outColor = vec4(pow(lighting.rgb, vec3(0.5)), 1.0);

}