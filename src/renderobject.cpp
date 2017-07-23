#include "renderobject.h"
#include "shader.h"

const char* mesh_vert_shader_text = "\n\
#version 430 core\n\
layout(location = 0) in vec3 position;\n\
layout(location = 1) in vec3 normal;\n\
layout(location = 2) in vec2 uv;\n\
out vec3 fragPos;\n\
out vec3 fragNorm;\n\
out vec2 fragUv;\n\
uniform mat4 MVP;\n\
void main() {\n\
	gl_Position = MVP * vec4(position, 1.0);\n\
	fragPos = gl_Position.xyz;\n\
	fragNorm = normal;\n\
	fragUv = uv;\n\
}\n\
";

static unsigned mesh_vert_handle = 0;

void Renderables::init(){
    tail = 0;
    if(!mesh_vert_handle){
        mesh_vert_handle = createShader(mesh_vert_shader_text, GL_VERTEX_SHADER);
    }
    prog.addShader(mesh_vert_handle);
    prog.addShader("write_to_gbuff.glsl", GL_FRAGMENT_SHADER);
    prog.link();
}

void GBuffer::init(int w, int h){
    screen.init();
    prog.init();
    lightbuff.init(9);

    glGenFramebuffers(1, &buff);
    glBindTexture(GL_FRAMEBUFFER, buff);

    glGenTextures(3, &posbuff);

    glBindTexture(GL_TEXTURE_2D, posbuff);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, posbuff, 0);

    glBindTexture(GL_TEXTURE_2D, normbuff);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, normbuff, 0);

    glBindTexture(GL_TEXTURE_2D, matbuff);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, matbuff, 0);
    
    unsigned attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    glDrawBuffers(3, attachments);

    prog.addShader(screen.vertexShader);
    prog.addShader("light_g_buff.glsl", GL_FRAGMENT_SHADER);
    prog.link();
}
void GBuffer::deinit(){
    screen.deinit();
    prog.deinit();
    lightbuff.deinit();
}

void GBuffer::updateLights(const LightSet& lights){
    lightbuff.upload(&lights, sizeof(LightSet));
}

void GBuffer::draw(const glm::vec3& eye){
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, posbuff);
    prog.setUniformInt("positionSampler", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normbuff);
    prog.setUniformInt("normalSampler", 1);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, matbuff);
    prog.setUniformInt("materialSampler", 2);
    prog.bind();
    prog.setUniform("eye", eye);
    screen.draw();
}

Renderables g_Renderables;

GBuffer g_gBuffer;