#include "gbuffer.h"
#include "camera.h"
#include "myglheaders.h"
#include "light.h"
#include "glscreen.h"
#include "renderobject.h"

void GBuffer::init(int w, int h){
    width = w;
    height = h;

    glGenFramebuffers(1, &buff);
    glBindFramebuffer(GL_FRAMEBUFFER, buff);

    glGenTextures(3, &posbuff);

    glBindTexture(GL_TEXTURE_2D, posbuff); DebugGL();;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL); DebugGL();;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); DebugGL();;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); DebugGL();;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, posbuff, 0); DebugGL();;

    glBindTexture(GL_TEXTURE_2D, normbuff); DebugGL();;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL); DebugGL();;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); DebugGL();;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); DebugGL();;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normbuff, 0); DebugGL();;

    glBindTexture(GL_TEXTURE_2D, matbuff); DebugGL();;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, NULL); DebugGL();;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); DebugGL();;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); DebugGL();;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, matbuff, 0); DebugGL();;
    
    unsigned attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments); DebugGL();;

    glGenRenderbuffers(1, &rboDepth); DebugGL();;
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth); DebugGL();;
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h); DebugGL();;
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth); DebugGL();;

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        puts("Framebuffer not complete!");
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0); DebugGL();;

    prog.init();
    prog.addShader(GLScreen::vertexShader());
    int shader = prog.addShader("light_g_buff.glsl", GL_FRAGMENT_SHADER);
    prog.link();
    prog.freeShader(shader);

    cmap.init(512);
}
void GBuffer::deinit(){
    prog.deinit();
    cmap.deinit();

    glDeleteRenderbuffers(1, &rboDepth);
    glDeleteTextures(3, &posbuff);
    glDeleteFramebuffers(1, &buff);
}

GBuffer g_gBuffer;

void GBuffer::draw(const Camera& cam, u32 dflag){
    static const int pos_loc = prog.getUniformLocation("positionSampler");
    static const int norm_loc = prog.getUniformLocation("normalSampler");
    static const int albedo_loc = prog.getUniformLocation("albedoSampler");

    static const int sundir_loc = prog.getUniformLocation("sunDirection");
    static const int suncolor_loc = prog.getUniformLocation("sunColor");
    static const int seed_loc = prog.getUniformLocation("seed");
    static const int eye_loc = prog.getUniformLocation("eye");
    static const int draw_flag_loc = prog.getUniformLocation("draw_flags");

    // draw into cubemap
    cmap.drawInto(cam);

    // draw into gbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, buff); DebugGL();;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();
    g_Renderables.defDraw(cam, cam.getVP(), width, height);

    // calculate lighting using gbuffer
    // replace this framebuffer with the post process buffer later
    glBindFramebuffer(GL_FRAMEBUFFER, 0); DebugGL();;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();;
    DepthContext dfctx(GL_ALWAYS);

    prog.bind();

    glActiveTexture(GL_TEXTURE0); DebugGL();;
    glBindTexture(GL_TEXTURE_2D, posbuff); DebugGL();;
    prog.setUniformInt(pos_loc, 0);

    glActiveTexture(GL_TEXTURE1); DebugGL();;
    glBindTexture(GL_TEXTURE_2D, normbuff); DebugGL();;
    prog.setUniformInt(norm_loc, 1);
    
    glActiveTexture(GL_TEXTURE2); DebugGL();;
    glBindTexture(GL_TEXTURE_2D, matbuff); DebugGL();;
    prog.setUniformInt(albedo_loc, 2);

    cmap.bind(20, prog);

    prog.setUniform(sundir_loc, g_Renderables.sunDirection);
    prog.setUniform(suncolor_loc, g_Renderables.sunColor);
    prog.setUniformFloat("sunIntensity", g_Renderables.sunIntensity);
    prog.setUniformInt(seed_loc, rand());
    prog.setUniform(eye_loc, cam.getEye());
    prog.setUniformInt(draw_flag_loc, dflag);
    prog.setUniform("render_resolution", glm::vec2(float(width), float(height)));
    prog.setUniform("IVP", cam.getIVP());

    GLScreen::draw();
}
