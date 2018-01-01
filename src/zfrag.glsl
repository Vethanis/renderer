#version 450 core

in vec3 Position;
out vec4 out_color;

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
    out_color = vec4(1.0);
}
