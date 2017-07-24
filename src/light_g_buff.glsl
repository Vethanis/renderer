#version 430 core
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

// ------------------------ main ------------------------------------

void main(){
    vec3 pos = texture(positionSampler, fragUv).rgb;
    vec3 N = texture(normalSampler, fragUv).rgb;
    vec3 albedo = texture(materialSampler, fragUv).rgb;
    float spec = texture(materialSampler, fragUv).a;

    vec3 lighting = albedo * 0.1;
    vec3 V = normalize(eye - pos);
    for(int i = 0; i < num_lights; i++){
        vec3 L = normalize(lights[i].position.xyz - pos);
	    vec3 H = normalize(V + L);
	    float D = max(0.0f, dot(L, N));
	    float S = (D > 0.0) ? pow(max(0.0, dot(H, N)), spec) : 0.0;
        float luminance = (S + D) / dot(lights[i].position.xyz - pos, lights[i].position.xyz - pos);
        lighting += luminance * albedo * lights[i].color.rgb;
    }

    outColor = vec4(lighting.rgb, 1.0);
}