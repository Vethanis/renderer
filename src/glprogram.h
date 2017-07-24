#pragma once

#include "glm/glm.hpp"
#include "store.h"

struct GLProgram{
    Store<int, 16> m_locations;
    static constexpr int max_shaders = 4;
    unsigned shader_handles[max_shaders];
    unsigned shader_tail;
    unsigned id;
    int getUniformLocation(unsigned name);
    void init();
    void deinit();
    void addShader(const char* path, int type);
    void addShader(unsigned handle);
    void link();
    void bind();
    void setUniform(unsigned name, const glm::vec2& v);
    void setUniform(unsigned name, const glm::vec3& v);
    void setUniform(unsigned name, const glm::vec4& v);
    void setUniform(unsigned name, const glm::mat3& v);
    void setUniform(unsigned name, const glm::mat4& v);
    void setUniformInt(unsigned name, const int v);
    void setUniformFloat(unsigned name, const float v);
};
