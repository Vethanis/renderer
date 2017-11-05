#include "gbuffer.h"
#include "camera.h"
#include "myglheaders.h"
#include "light.h"
#include "glscreen.h"
#include "renderobject.h"
#include "depthstate.h"
#include "randf.h"
#include "profiler.h"

unsigned draw_call = 0;

const char* progFilenames[2] = {
    "screenVert.glsl",
    "light_g_buff.glsl"
};
const char* postFilenames[2] = {
    "screenVert.glsl",
    "postFrag.glsl"
};

void GBuffer::init(int w, int h){
    width = w;
    height = h;

    m_framebuffer.init(w, h, 4);
    m_postbuffs[0].init(w, h, 1);
    m_postbuffs[1].init(w, h, 1);

    prog.setup(progFilenames, 2);
    postProg.setup(postFilenames, 2);

    cmap.init(512);
}
void GBuffer::deinit(){
    prog.deinit();
    postProg.deinit();
    cmap.deinit();

    m_framebuffer.deinit();
    m_postbuffs[0].deinit();
    m_postbuffs[1].deinit();
}

GBuffer g_gBuffer;

void GBuffer::draw(const Camera& cam, u32 dflag)
{
    ProfilerEvent("GBuffer::draw");

    static const int seed_loc = prog.getUniformLocation("seed");
    static const int eye_loc = prog.getUniformLocation("eye");
    static const int draw_flag_loc = prog.getUniformLocation("draw_flags");

    const float jitter_magnitude = 0.0001f;
    Transform VP = cam.getVP();
    Transform IVP = glm::inverse(VP);
    glm::vec3 jitter = randf(jitter_magnitude) * getRight(VP) + randf(jitter_magnitude) * getUp(VP);
    glm::vec3 eye = cam.getEye();
    eye += jitter;
    VP[3][0] += jitter.x;
    VP[3][1] += jitter.y;
    VP[3][2] += jitter.z;

    // SHADOWS pass ---------------------------------------------------------------------------
    g_Renderables.shadowPass();

    // CUBEMAP pass ---------------------------------------------------------------------------
    cmap.drawInto(cam);

    // WRITE pass -----------------------------------------------------------------------------
    m_framebuffer.bind();
    Framebuffer::clear();
    g_Renderables.defDraw(eye, VP, dflag, width, height);

    // LIGHTING pass ---------------------------------------------------------------------------
    Framebuffer& curBuf = m_postbuffs[draw_call & 1];
    Framebuffer& prevBuf = m_postbuffs[(draw_call + 1) & 1];

    {
        ProfilerEvent("GBuffer lighting pass");

        curBuf.bind();
        Framebuffer::clear();
        DepthContext dfctx(GL_ALWAYS);
        prog.bind();
    
        prog.bindTexture(0, m_framebuffer.m_attachments[0], "positionSampler");
        prog.bindTexture(1, m_framebuffer.m_attachments[1], "normalSampler");
        prog.bindTexture(2, m_framebuffer.m_attachments[2], "albedoSampler");
        prog.bindTexture(3, m_framebuffer.m_attachments[3], "velocitySampler");
        prog.bindTexture(4, prevBuf.m_attachments[0], "prevColor");
        prog.bindCubemap(5, cmap.color_cubemap, "env_cm");
        
        g_Renderables.bindSun(g_Renderables.m_light, prog, 10);
        prog.setUniformInt(seed_loc, rand());
        prog.setUniform(eye_loc, eye);
        prog.setUniformInt(draw_flag_loc, dflag);
        prog.setUniform("render_resolution", glm::vec2(float(width), float(height)));
        prog.setUniform("IVP", IVP);
        prog.setUniform("prevVP", cam.getPrevVP());
    
        glTextureBarrier(); DebugGL();
        GLScreen::draw();
    }

    // POST pass ---------------------------------------------------------------------------
    {
        ProfilerEvent("GBuffer post pass");
        
        Framebuffer::bindDefault();
        Framebuffer::clear();
        DepthContext dfctx(GL_ALWAYS);
    
        postProg.bind();
        postProg.bindTexture(0, curBuf.m_attachments[0], "curColor");
        postProg.setUniformInt("seed", rand());
    
        glTextureBarrier(); DebugGL();
        GLScreen::draw();
    }
    
    // -------------------------------------------------------------------------------------

    draw_call++;
}

void GBuffer::screenshot()
{
    char buff[64];
    snprintf(buff, sizeof(buff), "screenshots/Screenshot_%i.png", draw_call);
    m_postbuffs[draw_call & 1].saveToFile(buff, 0);
}
