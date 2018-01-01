#include "gbuffer.h"
#include "camera.h"
#include "myglheaders.h"
#include "glscreen.h"
#include "renderobject.h"
#include "depthstate.h"
#include "randf.h"
#include "profiler.h"
#include "framecounter.h"

unsigned draw_call = 0;

void GBuffer::init(int w, int h){
    width = w;
    height = h;

    m_framebuffer.init(w, h, 4);
    m_postbuff.init(w, h, 1);

    const char* progFilenames[2] = {
        "screenVert.glsl",
        "light_g_buff.glsl"
    };
    const char* postFilenames[2] = {
        "screenVert.glsl",
        "postFrag.glsl"
    };
    prog.setup(progFilenames, 2);
    postProg.setup(postFilenames, 2);

    cmap.init(512);
    GLScreen::init();
}
void GBuffer::deinit(){
    prog.deinit();
    postProg.deinit();
    cmap.deinit();

    m_framebuffer.deinit();
    m_postbuff.deinit();
}

GBuffer g_gBuffer;

void GBuffer::draw(const Camera& cam, u32 dflag, u32 target)
{
    ProfilerEvent("GBuffer::draw");

    static const int seed_loc = prog.getUniformLocation("seed");
    static const int eye_loc = prog.getUniformLocation("eye");
    static const int draw_flag_loc = prog.getUniformLocation("draw_flags");

    const Transform VP = cam.getVP();
    const Transform IVP = glm::inverse(VP);
    const glm::vec3 eye = cam.getEye();

    // WRITE pass -----------------------------------------------------------------------------
    {
        m_framebuffer.bind();
        Framebuffer::clear();
        g_Renderables.defDraw(eye, VP, dflag, width, height);
    }

    // LIGHTING pass ---------------------------------------------------------------------------
    Framebuffer& curBuf = m_postbuff;

    {
        ProfilerEvent("GBuffer::lighting_pass");

        curBuf.bind();
        Framebuffer::clear();
        DepthContext dfctx(GL_ALWAYS);
        prog.bind();
    
        prog.bindTexture(0, m_framebuffer.m_attachments[0], "positionSampler");
        prog.bindTexture(1, m_framebuffer.m_attachments[1], "normalSampler");
        prog.bindTexture(2, m_framebuffer.m_attachments[2], "albedoSampler");

        if(dflag != DF_DIRECT_CUBEMAP)
            prog.bindCubemap(5, cmap.color_cubemap, "env_cm");
        else
            prog.bindCubemap(5, 0, "env_cm");
        
        g_Renderables.bindSun(prog);
        prog.setUniformInt(seed_loc, rand());
        prog.setUniform(eye_loc, eye);
        prog.setUniformInt(draw_flag_loc, dflag);
        prog.setUniform("render_resolution", glm::vec2(float(width), float(height)));
        prog.setUniform("IVP", IVP);
        prog.setUniform("sunNearFar", glm::vec2(
            g_Renderables.m_light.m_near,
            g_Renderables.m_light.m_far
        ));
    
        glTextureBarrier(); DebugGL();
        GLScreen::draw();
    }

    // POST pass ---------------------------------------------------------------------------
    {
        ProfilerEvent("GBuffer::post_pass");
        
        glBindFramebuffer(GL_FRAMEBUFFER, target); DebugGL();
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
    m_postbuff.saveToFile(buff, 0);
}
