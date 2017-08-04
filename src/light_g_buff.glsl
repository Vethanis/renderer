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

// ------------------------ bvh ------------------------------------

#define OT_Depth 5
#define OT_IndexCapacity 31
#define OT_FaceCapacity 100000
#define OT_NodeCapacity (1 << (3 * OT_Depth))
#define OT_Radius 8.0

struct ot_vertex {
    vec4 position;
    vec4 normal;
    vec4 tangent;
};
struct ot_face {
    ot_vertex vertices[3];
};
struct ot_node {
    vec3 position; int face_tail;
    int children[8];
    int faces[OT_IndexCapacity];
    int child_tail;
};
struct ot_tail {
    int node;
    int face;
};

layout(binding=12) buffer face_buf{
    ot_face faces[];
};
layout(binding=13) buffer node_buf{
    ot_node nodes[];
};
layout(binding=14) buffer tail_buf{
    ot_tail tails;
};

// ------------------------ main ------------------------------------

float rand( inout uint f) {
    f = (f ^ 61) ^ (f >> 16);
    f *= 9;
    f = f ^ (f >> 4);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15);
    return fract(float(f) * 2.3283064e-10) * 2.0 - 1.0;
}

// moller-trumbore 
float ray_triangle_intersect(int face, vec3 orig, vec3 dir, out vec2 uv){
    vec3 v0 = faces[face].vertices[0].position.xyz;
    vec3 v1 = faces[face].vertices[1].position.xyz;
    vec3 v2 = faces[face].vertices[2].position.xyz;
    
    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;

    vec3 pvec = cross(dir, e2);
    float det = dot(e1, pvec);

    if(det < 1e-8 && det > -1e-8){
        return 0.0;
    }

    float inv_det = 1.0 / det;
    vec3 tvec = orig - v0;
    float u = dot(tvec, pvec) * inv_det;
    if(u < 0.0 || u > 1.0){
        return 0.0;
    }

    vec3 qvec = cross(tvec, e1);
    float v = dot(dir, qvec) * inv_det;
    if(v < 0.0 || u + v > 1.0){
        return 0.0;
    }

    uv = vec2(u, v);

    return dot(e2, qvec) * inv_det;
}

float intersect(int node, vec3 pos, vec3 dir, out int nFace, out vec2 uv){
    float t = OT_Radius * 10.0;
    for(int i = 0; i < nodes[node].face_tail; i++){
        vec2 nUv;
        float nt = ray_triangle_intersect(nodes[node].faces[i], pos, dir, nUv);
        if(nt > 0.0 && nt < t){
            t = nt;
            uv = nUv;
            nFace = nodes[node].faces[i];
        }
    }
    return t;
}

int find_first_face(vec3 pos, vec3 dir, out float t, out vec2 uv){
    t = OT_Radius * 10.0;
    int eval_tail = 0;
    int eval_head = 0;
    const int stack_size = 32;
    int stack_nodes[stack_size];
    stack_nodes[eval_head++] = 0;

    int face = -1;

    while(eval_head != eval_tail){
        int node = stack_nodes[eval_tail++];
        eval_tail &= (stack_size - 1);

        int nFace;
        vec2 nUv;
        float nt = intersect(node, pos, dir, nFace, nUv);
        if(nt > 0.0 && nt < t){
            t = nt;
            face = nFace;
            uv = nUv;
            for(int i = 0; i < nodes[node].child_tail; i++){
                stack_nodes[eval_head++] = nodes[node].children[i];
                eval_head &= (stack_size - 1);
            }
        }
    }

    return face;
}

vec3 sampleNormal(vec2 uv, int mat){
    return vec3(1.0);
}

vec3 sampleReflectance(vec2 uv, int mat){
    return vec3(1.0);
}

vec3 sampleEmittance(vec2 uv, int mat){
    return vec3(1.0);
}

vec3 trace(vec3 pos, vec3 dir){
    vec3 light = vec3(0.0);
    vec3 mask = vec3(1.0);

    float t = OT_Radius * 10.0;
    const int bounces = 3;
    for(int i = 0; i < bounces; i++){
        vec2 uv;
        int face = find_first_face(pos, dir, t, uv);
        if(face == -1)
            break;

        vec3 N = faces[face].vertices[0].normal.xyz;
        {
            vec3 e1 = faces[face].vertices[1].normal.xyz - N;
            vec3 e2 = faces[face].vertices[2].normal.xyz - N;
            N = normalize(N + uv.x * e1 + uv.y * e2);
        }

        vec2 txUv = vec2(faces[face].vertices[0].position.w, faces[face].vertices[0].normal.w);
        {
            vec2 uv1 = vec2(faces[face].vertices[1].position.w, faces[face].vertices[1].normal.w);
            vec2 uv2 = vec2(faces[face].vertices[2].position.w, faces[face].vertices[2].normal.w);

            vec2 e1 = uv1 - txUv;
            vec2 e2 = uv2 - txUv;

            txUv = txUv + uv.x * e1 + uv.y * e2;
        }

        vec3 T = faces[face].vertices[0].tangent.xyz;
        {
            vec3 e1 = faces[face].vertices[1].tangent.xyz - T;
            vec3 e2 = faces[face].vertices[2].tangent.xyz - T;

            T = normalize(T + uv.x * e1 + uv.y * e2);
        }

        vec3 B = cross(T, N);
        int mat = int(faces[face].vertices[0].tangent.w);

        mat3 TBN = mat3(T, B, N);

        N = normalize(TBN * normalize(sampleNormal(txUv, mat) * 2.0 - 1.0));
        vec3 reflectance = sampleReflectance(txUv, mat);
        vec3 emittance = sampleEmittance(txUv, mat);

        light += emittance * mask;
        mask *= reflectance;

        pos += dir * t;
        dir = normalize(reflect(-dir, N)); // change this to cos hemisphere sampling
        pos += dir * 0.001; // avoid re-intersection at new face
    }

    return light;
}

void main(){
    uint s = uint(seed) ^ uint(fragUv.x * 951489.0) ^ uint(fragUv.y * 7561182.0);
    vec3 pos = texture(positionSampler, fragUv).rgb;
    vec3 N = texture(normalSampler, fragUv).rgb;
    vec3 albedo = texture(materialSampler, fragUv).rgb;
    float spec = texture(materialSampler, fragUv).a * 128.0;

    vec3 V = normalize(eye - pos);
    vec3 dir = normalize(reflect(V, N)); // change this to cos hemisphere sampling
    pos += dir * 0.001;

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