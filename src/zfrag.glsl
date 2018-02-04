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
    ivec4 seed_flags; // x -> seed, y -> draw mode
    ivec4 sampler_states; // x -> env_cm; y -> sunDepth;
};

layout(std430, binding = SHARED_UNIFORMS_BINDING) buffer SU_BUFFER
{
    SharedUniforms SU;
};

uniform sampler3D distance_field;

float get_distance(vec3 pt)
{
    pt -= SU.df_translation.xyz;
    pt /= SU.df_scale.xyz;
    return texture(distance_field, pt).r;
}

void main()
{
    #if 0

    const vec3 rd = normalize(Position.xyz - SU.eye.xyz);
    vec3 pt = SU.eye.xyz;
    
    {
        float dis = 1000.0;
        for(int i = 0; i < 30; ++i)
        {
            dis = get_distance(pt);
            if(dis < SU.df_translation.w)
            {
                break;
            }
            pt += rd * dis;
        }

        if(dis > SU.df_translation.w)
        {
            discard;
            return;
        }
    }

    // replace this with newtons
    vec3 A = pt - rd * SU.df_translation.w;
    vec3 B = pt + rd * SU.df_translation.w;
    for(int i = 0; i < 8; ++i)
    {
        pt = (A + B) * 0.5;
        const float dis = abs(get_distance(pt));
        const float dA  = abs(get_distance(A));
        const float dB  = abs(get_distance(B));
        if(dA < dis)
        {
            B = pt;
        }
        else if(dB < dis)
        {
            A = pt;
        }
        else
        {
            break;
        }
    }

    vec4 scrPt = SU.MVP * vec4(pt.xyz, 1.0);
    scrPt /= scrPt.w;
    gl_FragDepth = scrPt.z;

    #endif

    Color = vec4(1.0);
}
