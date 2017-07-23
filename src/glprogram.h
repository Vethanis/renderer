#pragma once

#include "glm/glm.hpp"
#include "store.h"

struct GLProgram{
    Store<int, 16> m_locations;
    static constexpr int max_shaders = 4;
    unsigned shader_handles[max_shaders];
    unsigned shader_tail;
    unsigned id;
    int getUniformLocation(const char* name);
    void init();
    void deinit();
    void addShader(const char* path, int type);
    void addShader(unsigned handle);
    void link();
    void bind();
    void setUniform(const char* name, const glm::vec2& v);
    void setUniform(const char* name, const glm::vec3& v);
    void setUniform(const char* name, const glm::vec4& v);
    void setUniform(const char* name, const glm::mat3& v);
    void setUniform(const char* name, const glm::mat4& v);
    void setUniformInt(const char* name, const int v);
    void setUniformFloat(const char* name, const float v);
};
