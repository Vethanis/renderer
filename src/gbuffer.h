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
    Framebuffer m_postbuff;
    unsigned width, height;
    void init(int w, int h);
    void deinit();
    void drawCubemap(const Camera& cam){ cmap.draw(cam); }
    void draw(const Camera& cam, u32 dflag, u32 target = 0);
    void screenshot();
};

extern GBuffer g_gBuffer;