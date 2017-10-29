#pragma once

#include "glprogram.h"
#include "cubemap.h"
#include "framebuffer.h"

class Camera;

struct GBuffer
{
    GLProgram prog;
    GLProgram postProg;
    Cubemap cmap;
    Framebuffer m_framebuffer;
    Framebuffer m_postbuffs[2];
    unsigned width, height;
    void init(int w, int h);
    void deinit();
    void draw(const Camera& cam, u32 dflag);
    void screenshot();
};

extern GBuffer g_gBuffer;