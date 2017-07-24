#pragma once

#include "glm/glm.hpp"
#include "store.h"

struct GLProgram{
    unsigned id;
    void init();
    void deinit();
    int addShader(const char* path, int type);
    void addShader(unsigned handle);
    void freeShader(int id);
    void link();
    void bind();
    int getUniformLocation(const char* name);
    void setUniform(int loc, const glm::vec2& v);
    void setUniform(int loc, const glm::vec3& v);
    void setUniform(int loc, const glm::vec4& v);
    void setUniform(int loc, const glm::mat3& v);
    void setUniform(int loc, const glm::mat4& v);
    void setUniformInt(int loc, const int v);
    void setUniformFloat(int loc, const float v);
};
