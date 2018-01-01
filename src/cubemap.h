#pragma once

#include "ints.h"

struct GLProgram;
class Camera;

struct Cubemap
{
    static constexpr u32 num_faces = 6;
    u32 fbo;
    u32 rbo;
    u32 color_cubemap;
    u32 current_face;
    s32 m_size;
    void init(s32 size);
    void deinit();
    void draw(const Camera& cam);
};