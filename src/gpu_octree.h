#pragma once 

#include "SSBO.h"
#include "vertexbuffer.h"

constexpr inline int num_nodes(int d){
    int num = 1;
    for(int i = 1; i <= d; i++){
        num += 1 << (3 * i);
    }
    return num;
}

struct GpuOctree{
    static constexpr int depth = 5;
    static constexpr int node_count = num_nodes(depth);
    static constexpr int leaf_count = 1 << (3 * depth);
    static constexpr int leaf_capacity = 256;

    struct face {
        Vertex vertices[3];
    };

    struct leaf {
        face faces[leaf_capacity];
    };

    leaf leaves[leaf_count];

    int tail = 0;
};