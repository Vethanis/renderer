#pragma once

#include "vertexbuffer.h"

struct Mesh 
{
    u32 vao, vbo, num_indices;
    void draw()const;
    void upload(const Geometry& geom);
    void init();
    void deinit();
};