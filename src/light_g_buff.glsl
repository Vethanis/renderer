#version 450 core
out vec4 outColor;

in vec2 fragUv;

// ------------------------- samplers ---------------------------------

uniform sampler2D positionSampler;
uniform sampler2D normalSampler;
uniform sampler2D materialSampler;

// -------------------------- lights ----------------------------------

struct light {
    vec4 position;
    vec4 color;
};
const int num_lights = 32;
layout(binding=9) buffer light_buf {
    light lights[num_lights];
};

// ------------------------ uniforms --------------------------------

uniform vec3 eye;
uniform vec3 forward;
uniform int seed;

// ------------------------ main ------------------------------------

float rand( inout uint f) {
    f = (f ^ 61) ^ (f >> 16);
    f *= 9;
    f = f ^ (f >> 4);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15);
    return fract(float(f) * 2.3283064e-10) * 2.0 - 1.0;
}

void main(){
    uint s = uint(seed) ^ uint(fragUv.x * 951489.0) ^ uint(fragUv.y * 7561182.0);
    vec3 pos = texture(positionSampler, fragUv).rgb;
    vec3 N = texture(normalSampler, fragUv).rgb;
    vec3 albedo = texture(materialSampler, fragUv).rgb;
    float spec = texture(materialSampler, fragUv).a * 128.0;

    //outColor = vec4(N * 0.5 + 0.5, 1.0);
    //return;

    vec3 V = normalize(eye - pos);
    vec3 lighting = albedo * 0.01;

    for(int i = 0; i < num_lights; i++){
        vec3 L = normalize(lights[i].position.xyz - pos);
	    vec3 H = normalize(V + L);
	    float D = max(0.0, dot(L, N));
	    float S = (D > 0.0) ? pow(max(0.0, dot(H, N)), spec) : 0.0;
        float luminance = (S + D) / dot(lights[i].position.xyz - pos, lights[i].position.xyz - pos);
        lighting += luminance * albedo * lights[i].color.rgb;
    }

    lighting.rgb.x += 0.0001 * rand(s);
    lighting.rgb.y += 0.0001 * rand(s);
    lighting.rgb.z += 0.0001 * rand(s);

    outColor = vec4(pow(lighting.rgb, vec3(0.5)), 1.0);
}