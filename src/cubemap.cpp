#include "cubemap.h"
#include "myglheaders.h"
#include "renderobject.h"
#include "camera.h"
#include "glprogram.h"

void Cubemap::init(s32 size){
    current_face = 0;

    glGenTextures(1, &depth_cubemap); DebugGL();
    glBindTexture(GL_TEXTURE_CUBE_MAP, depth_cubemap); DebugGL();
    for(u32 i = 0; i < num_faces; ++i){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 
            GL_DEPTH_COMPONENT, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr); DebugGL();
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); DebugGL();

    glGenTextures(1, &color_cubemap); DebugGL();
    glBindTexture(GL_TEXTURE_CUBE_MAP, color_cubemap); DebugGL();
    for(u32 i = 0; i < num_faces; ++i){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 
            GL_RGBA, size, size, 0, GL_RGBA, GL_FLOAT, nullptr); DebugGL();
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); DebugGL();

    glGenFramebuffers(num_faces, fbos); DebugGL();
    for(u32 i = 0; i < num_faces; ++i){
        glBindFramebuffer(GL_FRAMEBUFFER, fbos[i]); DebugGL();
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depth_cubemap, 0, i); DebugGL();
        glFramebufferTextureLayer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, color_cubemap, 0, i); DebugGL();
        glDrawBuffer(GL_COLOR_ATTACHMENT0); DebugGL();    
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
            puts("Framebuffer not complete!");
            assert(false);
        }
    }
}

void Cubemap::deinit(){
    glDeleteFramebuffers(num_faces, fbos);
    glDeleteTextures(1, &color_cubemap);
    glDeleteTextures(1, &depth_cubemap);
}

void Cubemap::bind(u32 channel, GLProgram& prog){
    glActiveTexture(GL_TEXTURE0 + channel); DebugGL();
    glBindTexture(GL_TEXTURE_CUBE_MAP, color_cubemap); DebugGL();
    prog.setUniformInt("env_cm", channel);
}

void Cubemap::drawInto(const Camera& cam){
    static const Transform Vs[num_faces] = {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f))
    };
    static const Transform P = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0); DebugGL();
    glBindFramebuffer(GL_FRAMEBUFFER, fbos[current_face]); DebugGL();
    const Transform VP = P * glm::translate(Vs[current_face], cam.getEye());
    g_Renderables.fwdDraw(cam, VP, DF_CUBEMAP | DF_DIRECT);
 
    glBindTexture(GL_TEXTURE_CUBE_MAP, color_cubemap); DebugGL();
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP); DebugGL();

    current_face = (current_face + 1) % 6;
}