#version 450 core

out vec4 outColor;
in vec2 fragUv;

uniform sampler2D curColor;
uniform int seed;

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

void main()
{
    uint s = uint(seed) 
        ^ uint(gl_FragCoord.x * 39163.0) 
        ^ uint(gl_FragCoord.y * 64601.0);
    vec3 lighting = texture(curColor, fragUv).rgb;

    lighting.rgb.x += 0.0005 * randBi(s);
    lighting.rgb.y += 0.0005 * randBi(s);
    lighting.rgb.z += 0.0005 * randBi(s);
    lighting.rgb = pow(lighting.rgb, vec3(1.0 / 2.2));

    outColor = vec4(lighting.rgb, 1.0);
}