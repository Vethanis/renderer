#pragma once

#include "common.h"

struct GLProgram;
class Camera;

struct Cubemap{
    static constexpr u32 num_faces = 6;
    u32 fbos[num_faces];
    u32 color_cubemap;
    u32 depth_cubemap;
    u32 current_face;
    void init(s32 size);
    void deinit();
    void bind(u32 channel, GLProgram& prog);
    void drawInto(const Camera& cam);
};