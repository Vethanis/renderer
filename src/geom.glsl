#version 450

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

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

in mat3 gTBN[];
in vec3 gP[];
in vec2 gUV[];
flat in int gMID[];

out mat3 TBN;
out vec3 P;
out vec2 UV;
flat out int MID;

vec3 child_pos(vec3 pos, int i, float radius){
    pos.x += (i & 1) != 0 ? -radius : radius;
    pos.y += (i & 2) != 0 ? -radius : radius;
    pos.z += (i & 4) != 0 ? -radius : radius;
    return pos;
}

void push_face(int node){
    if(nodes[node].face_tail < OT_IndexCapacity){
        int g_back = atomicAdd(tails.face, 1);
        int node_back = atomicAdd(nodes[node].face_tail, 1);
        if(node_back < OT_IndexCapacity){
            nodes[node].faces[node_back] = g_back;
            for(int i = 0; i < 3; i++){
                faces[g_back].vertices[i].position.xyz = gP[i];
                faces[g_back].vertices[i].position.w = gUV[i].x;
                faces[g_back].vertices[i].normal.xyz = gTBN[i][2];
                faces[g_back].vertices[i].normal.w = gUV[i].y;
                faces[g_back].vertices[i].tangent.xyz = gTBN[i][0];
                faces[g_back].vertices[i].tangent.w = float(gMID[i]);
            }
        }
    }
}

void push_child(int parent, float rad){
    int octant = atomicAdd(nodes[parent].child_tail, 1);
    if(octant < 8){
        int child = atomicAdd(tails.node, 1);
        nodes[parent].children[octant] = child;
        nodes[child].position = child_pos(nodes[parent].position, octant, rad);
    }
}

bool contains(vec3 position, float rad, vec3 lo, vec3 hi){
    vec3 bound = position - rad;
    if(lo.x < bound.x || lo.y < bound.y || lo.z < bound.z)
        return false;
    bound = position + rad;
    if(hi.x > bound.x || hi.y > bound.y || hi.z > bound.z)
        return false;
    return true;
}

void main() {
    // insert face into octree
    {
        int node = 0;
        float node_radius = OT_Radius;
        vec3 face_min = min(min(gP[0], gP[1]), gP[2]);
        vec3 face_max = max(max(gP[0], gP[1]), gP[2]);
        float face_rad = distance(face_max, face_min);
        if(!contains(nodes[node].position, node_radius, face_min, face_max))
            return;
        
        push_face(node);

        const int stack_size = 32;
        int evals[stack_size];
        float radii[stack_size];
        int eval_head = 0;
        int eval_tail = 0;
        evals[eval_head] = node;
        radii[eval_head] = node_radius;
        eval_head++;

        while(eval_head != eval_tail)
        {
            float rad = 0.5 * radii[eval_tail];
            int cur_node = evals[eval_tail];
            eval_tail = (eval_tail + 1) & (stack_size - 1);

            if(face_rad > rad)
                continue;

            for(int i = 0; i < 8; i++){
                if(nodes[cur_node].child_tail < 8){
                    push_child(cur_node, rad);
                }
                int child = nodes[cur_node].children[i];
                if(contains(nodes[child].position, rad, face_min, face_max)){
                    push_face(child);
                    evals[eval_head] = child;
                    radii[eval_head] = rad;

                    eval_head = (eval_head + 1) & (stack_size - 1);
                }
            }
        }

    }

    // passthrough...
    for(int i = 0; i < 3; i++){
        gl_Position = gl_in[i].gl_Position;
        TBN = gTBN[i];
        P = gP[i];
        UV = gUV[i];
        MID = gMID[i];
        EmitVertex();
    }
    EndPrimitive();
}