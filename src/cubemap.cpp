#include "cubemap.h"
#include "myglheaders.h"
#include "renderobject.h"
#include "camera.h"
#include "glprogram.h"

void Cubemap::init(s32 size){
    current_face = 0;
    m_size = size;
    
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS); 
    glGenTextures(1, &color_cubemap); DebugGL();
    glBindTexture(GL_TEXTURE_CUBE_MAP, color_cubemap); DebugGL();
    for(u32 i = 0; i < num_faces; ++i){
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, 
            GL_RGBA32F, m_size, m_size, 0, GL_RGBA, GL_FLOAT, nullptr); DebugGL();
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); DebugGL();
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE); DebugGL();
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP); DebugGL();

    glGenFramebuffers(1, &fbo); DebugGL();
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); DebugGL();
    glGenRenderbuffers(1, &rbo); DebugGL();
    glBindRenderbuffer(GL_RENDERBUFFER, rbo); DebugGL();
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 
        m_size, m_size); DebugGL();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, color_cubemap, 0); DebugGL();
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, 
        GL_RENDERBUFFER, rbo); DebugGL();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, 
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, color_cubemap, 0); DebugGL();

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        puts("[OpenGL] Incomplete framebuffer.");
        assert(false);
    }
}

void Cubemap::deinit(){
    glDeleteFramebuffers(1, &fbo);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteTextures(1, &color_cubemap);
}

void Cubemap::drawInto(const Camera& cam){
    static const Transform Vs[num_faces] = {
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
    };
    static const Transform P = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);  

    glBindTexture(GL_TEXTURE_CUBE_MAP, 0); DebugGL(); // dont want to draw and read same buffer, so bind null cubemap
    glBindFramebuffer(GL_FRAMEBUFFER, fbo); DebugGL();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + current_face, color_cubemap, 0); DebugGL();

    const Transform VP = P * glm::translate(Vs[current_face], -cam.getEye());
    g_Renderables.fwdDraw(cam, VP, DF_DIRECT_CUBEMAP, m_size, m_size);

    glBindTexture(GL_TEXTURE_CUBE_MAP, color_cubemap); DebugGL();
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP); DebugGL();
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0); DebugGL();

    current_face = (current_face + 1) % num_faces;
}