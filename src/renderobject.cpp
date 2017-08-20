#include "renderobject.h"
#include "shader.h"

void Renderables::init(){
    prog.init();
    int vert = prog.addShader("vert.glsl", GL_VERTEX_SHADER);
    //int geom = prog.addShader("geom.glsl", GL_GEOMETRY_SHADER);
    int frag = prog.addShader("write_to_gbuff.glsl", GL_FRAGMENT_SHADER);
    prog.link();
    prog.freeShader(vert);
    //prog.freeShader(geom);
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

    glBindTexture(GL_TEXTURE_2D, posbuff); DebugGL();;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL); DebugGL();;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); DebugGL();;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); DebugGL();;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, posbuff, 0); DebugGL();;

    glBindTexture(GL_TEXTURE_2D, normbuff); DebugGL();;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL); DebugGL();;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); DebugGL();;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); DebugGL();;
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normbuff, 0); DebugGL();;

    glBindTexture(GL_TEXTURE_2D, matbuff); DebugGL();;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL); DebugGL();;
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
}
void GBuffer::deinit(){
    prog.deinit();
    lightbuff.deinit();
}

void GBuffer::updateLights(const LightSet& lights){
    lightbuff.upload(lights.data, lights.bytes());
}

Renderables g_Renderables;
GBuffer g_gBuffer;

void GBuffer::draw(const Camera& cam){
    static const int mat_name = prog.getUniformLocation("materialSampler");
    static const int seed_name = prog.getUniformLocation("seed");
    static const int eye_name = prog.getUniformLocation("eye");
    static const int pos_name = prog.getUniformLocation("positionSampler");
    static const int norm_name = prog.getUniformLocation("normalSampler");

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f); DebugGL();;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();;

    // draw into gbuffer
    glBindFramebuffer(GL_FRAMEBUFFER, buff); DebugGL();;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();;
    g_Renderables.draw(cam.getVP());
    glBindFramebuffer(GL_FRAMEBUFFER, 0); DebugGL();;

    // calculate lighting using gbuffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();;
    prog.bind();
    glActiveTexture(GL_TEXTURE0); DebugGL();;
    glBindTexture(GL_TEXTURE_2D, posbuff); DebugGL();;
    prog.setUniformInt(pos_name, 0);
    glActiveTexture(GL_TEXTURE1); DebugGL();;
    glBindTexture(GL_TEXTURE_2D, normbuff); DebugGL();;
    prog.setUniformInt(norm_name, 1);
    glActiveTexture(GL_TEXTURE2); DebugGL();;
    glBindTexture(GL_TEXTURE_2D, matbuff); DebugGL();;
    prog.setUniformInt(mat_name, 2);
    prog.setUniformInt(seed_name, rand());
    prog.setUniform(eye_name, cam.getEye());
    GLScreen::draw();

    // copy geom's zbuff to default zbuff
    // glBindFramebuffer(GL_READ_FRAMEBUFFER, buff); DebugGL();;
    // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);  DebugGL();; // write to default framebuffer
    // glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST); DebugGL();;
    // glBindFramebuffer(GL_FRAMEBUFFER, 0); DebugGL();;

}
