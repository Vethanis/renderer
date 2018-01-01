#version 450 core

layout (location = 0) out vec4 gPosition; //   a: roughness
layout (location = 1) out vec4 gNormal;   //   a: metalness
layout (location = 2) out vec4 gAlbedo;   // rgb: albedo,   a: ao

in vec3 Position;

uniform mat4 MVP;
uniform vec3 eye;
uniform vec3 df_translation;
uniform vec3 df_scale;
uniform float df_pitch;
uniform sampler3D distance_field;

float get_distance(vec3 pt)
{
    pt -= df_translation;
    pt /= df_scale;
    return texture(distance_field, pt).r;
}

vec3 get_normal(vec3 pt)
{
    const vec3 e = vec3(0.001, 0.0, 0.0);
    return normalize(
        vec3(
            get_distance(pt + e.xyz) - get_distance(pt - e.xyz),
            get_distance(pt + e.yxz) - get_distance(pt - e.yxz),
            get_distance(pt + e.zyx) - get_distance(pt - e.zyx)
        )
    );
}

vec3 get_color(vec3 pt)
{
    return vec3(1.0, 0.0, 0.0); // NYI
}

vec3 get_material(vec3 pt)
{
    return vec3(0.5, 0.0, 0.0); // NYI
}

void main()
{
    const vec3 rd = normalize(Position.xyz - eye.xyz);
    vec3 pt = eye;
    
    {
        float dis = 1000.0;
        for(int i = 0; i < 30; ++i)
        {
            dis = get_distance(pt);
            if(dis < df_pitch)
            {
                break;
            }
            pt += rd * dis;
        }

        if(dis > df_pitch)
        {
            discard;
            return;
        }
    }

    vec3 A = pt - rd * df_pitch;
    vec3 B = pt + rd * df_pitch;
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

    vec4 scrPt = MVP * vec4(pt.xyz, 1.0);
    scrPt /= scrPt.w;
    gl_FragDepth = scrPt.z;

    gPosition.xyz = pt.xyz;
    gNormal.xyz = get_normal(pt);
    gAlbedo.rgb = get_color(pt);
    
    const vec3 mat = get_material(pt);
    gPosition.w = mat.x;
    gNormal.w = mat.y;
    gAlbedo.w = mat.z;
}
