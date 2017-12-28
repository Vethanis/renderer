#pragma once

#include "ints.h"
#include "vertexbuffer.h"

struct Mesh 
{
    u32 vao, vbo, num_indices;
    void draw()const;
    void upload(const Vertex* p, const u32 count);
    void init();
    void deinit();
};