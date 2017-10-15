#pragma once

#include "glm/glm.hpp"

struct DirectionalLight
{
    glm::mat4 m_matrix;
    glm::vec3 m_position;
    glm::vec3 m_direction;
    glm::vec3 m_color;
    float m_intensity;
    float m_near;
    float m_far;
    unsigned m_fbo;
    unsigned m_tex;
    int m_size;

    void init(int size = 1024);
    void deinit();
    void bindTexture(int channel);
    void drawInto();
};