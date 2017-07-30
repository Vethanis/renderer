#pragma once 

#include "SSBO.h"
#include "vertexbuffer.h"

namespace oct {
    using namespace glm;

    #define OT_Depth 4
    #define OT_IndexCapacity 31
    #define OT_FaceCapacity 100000
    #define OT_NodeCapacity (1 << (3 * OT_Depth))
    #define OT_Radius 32.0

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

    struct gpu_octree {
        ot_face faces[OT_FaceCapacity];
        ot_node nodes[OT_NodeCapacity];
        ot_tail tails;

        SSBO face_buf, node_buf, tail_buf;

        void reset(){
            nodes[0].position = vec3(0.0f);
            tails.node = 1;
            tails.face = 0;
            for(ot_node& node : nodes){
                node.face_tail = 0;
                node.child_tail = 0;
                for(int i = 0; i < 8; i++){
                    node.children[i] = -1;
                }
            }
        }
        void init(int binding=12){
            reset();
            face_buf.init(binding);
            node_buf.init(binding+1);
            tail_buf.init(binding+2);
        }
        void deinit(){
            face_buf.deinit();
            node_buf.deinit();
            tail_buf.deinit();
        }
        void upload(){
            face_buf.upload(faces, sizeof(ot_face) * OT_FaceCapacity);
            node_buf.upload(nodes, sizeof(ot_node) * OT_NodeCapacity);
            tail_buf.upload(&tails, sizeof(ot_tail));
        }
    };

}; // namespace oct