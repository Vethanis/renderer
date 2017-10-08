#pragma once

#include "glprogram.h"
#include "cubemap.h"

class Camera;

struct GBuffer{
    unsigned buff;
    unsigned rboDepth;
    unsigned posbuff, normbuff, matbuff;
    unsigned width, height;
    Cubemap cmap;
    GLProgram prog;
    void init(int w, int h);
    void deinit();
    void draw(const Camera& cam, u32 dflag);
};

extern GBuffer g_gBuffer;