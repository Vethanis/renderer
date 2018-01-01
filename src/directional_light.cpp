#include "directional_light.h"
#include "myglheaders.h"
#include "glm/gtc/matrix_transform.hpp"
#include "renderobject.h"
#include "glprogram.h"
#include "camera.h"

void DirectionalLight::init(int size)
{
    m_size = size;

    glGenTextures(1, &m_tex);  DebugGL();
    glBindTexture(GL_TEXTURE_2D, m_tex);  DebugGL();
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, size, size, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);  DebugGL();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);  DebugGL();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);  DebugGL();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);  DebugGL();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);  DebugGL();

    glGenFramebuffers(1, &m_fbo);  DebugGL();
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);  DebugGL();
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_tex, 0);  DebugGL();
    glDrawBuffer(GL_NONE);  DebugGL();
    glReadBuffer(GL_NONE);  DebugGL();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);  DebugGL();
}

void DirectionalLight::deinit()
{
    glDeleteTextures(1, &m_tex);
    glDeleteFramebuffers(1, &m_fbo);
}

void DirectionalLight::bind(GLProgram& prog, int channel)
{
    prog.setUniform("sunDirection", m_direction);
    prog.setUniform("sunColor", m_color);
    prog.setUniformFloat("sunIntensity", m_intensity);
    prog.setUniform("sunMatrix", m_matrix);
    prog.bindTexture(channel, m_tex, "sunDepth");
}

void DirectionalLight::drawInto(const Camera& cam)
{
    glViewport(0, 0, m_size, m_size);  DebugGL();
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);  DebugGL();
    glCullFace(GL_FRONT);  DebugGL();

    m_matrix = glm::perspective(glm::radians(60.0f), 1.0f, m_near, m_far) 
        * glm::lookAt(m_position + m_direction, m_position, glm::vec3(0.0f, 1.0f, 0.0f));

    g_Renderables.depthPass(cam.getEye(), m_matrix);
    glCullFace(GL_BACK);  DebugGL();
}