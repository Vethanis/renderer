#include "renderobject.h"
#include "shader.h"

void Renderables::init(){
    tail = 0;
    prog.init();
    int vert = prog.addShader("vert.glsl", GL_VERTEX_SHADER);
    int geom = prog.addShader("geom.glsl", GL_GEOMETRY_SHADER);
    int frag = prog.addShader("write_to_gbuff.glsl", GL_FRAGMENT_SHADER);
    prog.link();
    prog.freeShader(vert);
    prog.freeShader(geom);
    prog.freeShader(frag);
    tree.init();
}

void GBuffer::init(int w, int h){
    width = w;
    height = h;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    lightbuff.init(9);

    glGenFramebuffers(1, &buff);
    glBindFramebuffer(GL_FRAMEBUFFER, buff);

    glGenTextures(3, &posbuff);

    glBindTexture(GL_TEXTURE_2D, posbuff); MYGLERRORMACRO;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL); MYGLERRORMACRO;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); MYGLERRORMACRO;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); MYGLERRORMACRO;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, posbuff, 0); MYGLERRORMACRO;

    glBindTexture(GL_TEXTURE_2D, normbuff); MYGLERRORMACRO;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL); MYGLERRORMACRO;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); MYGLERRORMACRO;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); MYGLERRORMACRO;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normbuff, 0); MYGLERRORMACRO;

    glBindTexture(GL_TEXTURE_2D, matbuff); MYGLERRORMACRO;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); MYGLERRORMACRO;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); MYGLERRORMACRO;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); MYGLERRORMACRO;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, matbuff, 0); MYGLERRORMACRO;
    
    unsigned attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments); MYGLERRORMACRO;

    glGenRenderbuffers(1, &rboDepth); MYGLERRORMACRO;
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth); MYGLERRORMACRO;
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, w, h); MYGLERRORMACRO;
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth); MYGLERRORMACRO;

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        puts("Framebuffer not complete!");
        assert(false);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0); MYGLERRORMACRO;

    prog.init();
    prog.addShader(GLScreen::vertexShader());
    int shader = prog.addShader("light_g_buff.glsl", GL_FRAGMENT_SHADER);
    prog.link();
    prog.freeShader(shader);
}
void GBuffer::deinit(){
    prog.deinit();
    lightbuff.deinit();
}

void GBuffer::updateLights(const LightSet& lights){
    lightbuff.upload(&lights, sizeof(LightSet));
}

Renderables g_Renderables;
GBuffer g_gBuffer;

void GBuffer::draw(const Camera& cam){
    static const int mat_name = prog.getUniformLocation("materialSampler");
    static const int seed_name = prog.getUniformLocation("seed");
    static const int eye_name = prog.getUniformLocation("eye");
    static const int forward_name = prog.getUniformLocation("forward");
    static const int pos_name = prog.getUniformLocation("positionSampler");
    static const int norm_name = prog.getUniformLocation("normalSampler");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); MYGLERRORMACRO;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); MYGLERRORMACRO;

    // draw into gbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, buff); MYGLERRORMACRO;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); MYGLERRORMACRO;
    g_Renderables.draw(cam.getVP());
    glBindFramebuffer(GL_FRAMEBUFFER, 0); MYGLERRORMACRO;

    // calculate lighting using gbuffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); MYGLERRORMACRO;
    prog.bind();
    glActiveTexture(GL_TEXTURE0); MYGLERRORMACRO;
    glBindTexture(GL_TEXTURE_2D, posbuff); MYGLERRORMACRO;
    prog.setUniformInt(pos_name, 0);
    glActiveTexture(GL_TEXTURE1); MYGLERRORMACRO;
    glBindTexture(GL_TEXTURE_2D, normbuff); MYGLERRORMACRO;
    prog.setUniformInt(norm_name, 1);
    glActiveTexture(GL_TEXTURE2); MYGLERRORMACRO;
    glBindTexture(GL_TEXTURE_2D, matbuff); MYGLERRORMACRO;
    prog.setUniformInt(mat_name, 2);
    prog.setUniformInt(seed_name, rand());
    prog.setUniform(eye_name, cam.getEye());
    prog.setUniform(forward_name, cam.getAxis());
    GLScreen::draw();

    // copy geom's zbuff to default zbuff
    // glBindFramebuffer(GL_READ_FRAMEBUFFER, buff); MYGLERRORMACRO;
    // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);  MYGLERRORMACRO; // write to default framebuffer
    // glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST); MYGLERRORMACRO;
    // glBindFramebuffer(GL_FRAMEBUFFER, 0); MYGLERRORMACRO;

}
