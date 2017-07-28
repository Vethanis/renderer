#include "renderobject.h"
#include "shader.h"

const char* mesh_vert_shader_text = "\n\
#version 450 core\n\
layout(location = 0) in vec4 posu;\n\
layout(location = 1) in vec4 norv;\n\
layout(location = 2) in vec4 tm;\n\
layout(location = 3) in vec4 b;\n\
out vec3 fragPos;\n\
out vec3 fragNorm;\n\
out vec2 fragUv;\n\
flat out int fragChannel;\n\
out mat3 TBN;\n\
uniform mat4 MVP;\n\
uniform mat4 M;\n\
uniform mat3 IM;\n\
void main() {\n\
	gl_Position = MVP * vec4(posu.xyz, 1.0);\n\
	fragPos = vec3(M * vec4(posu.xyz, 1.0));\n\
	fragNorm = IM * norv.xyz;\n\
	fragUv = vec2(posu.w, norv.w);\n\
    fragChannel = int(tm.w);\n\
    vec3 T = normalize(vec3(M * vec4(IM * tm.xyz, 0.0)));\n\
    vec3 B = normalize(vec3(M * vec4(IM * b.xyz, 0.0)));\n\
    vec3 N = normalize(vec3(M * vec4(IM * norv.xyz, 0.0)));\n\
    TBN = mat3(T, B, N);\n\
}\n\
";

static unsigned mesh_vert_handle = 0;

void Renderables::init(){
    tail = 0;
    if(!mesh_vert_handle){
        mesh_vert_handle = createShader(mesh_vert_shader_text, GL_VERTEX_SHADER);
    }
    prog.init();
    prog.addShader(mesh_vert_handle);
    int shader = prog.addShader("write_to_gbuff.glsl", GL_FRAGMENT_SHADER);
    prog.link();
    prog.freeShader(shader);
}

void GBuffer::init(int w, int h){
    width = w;
    height = h;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    screen.init();
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
    prog.addShader(screen.vertexShader);
    int shader = prog.addShader("light_g_buff.glsl", GL_FRAGMENT_SHADER);
    prog.link();
    prog.freeShader(shader);
}
void GBuffer::deinit(){
    screen.deinit();
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
    screen.draw();

    // copy geom's zbuff to default zbuff
    // glBindFramebuffer(GL_READ_FRAMEBUFFER, buff); MYGLERRORMACRO;
    // glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);  MYGLERRORMACRO; // write to default framebuffer
    // glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_DEPTH_BUFFER_BIT, GL_NEAREST); MYGLERRORMACRO;
    // glBindFramebuffer(GL_FRAMEBUFFER, 0); MYGLERRORMACRO;

}
