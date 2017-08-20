#pragma once

#include "glm/glm.hpp"
#include "store.h"
#include "hashstring.h"

struct GLProgram{
    unsigned id;
    Store<int, 32> locations;
    void init();
    void deinit();
    int addShader(const char* path, int type);
    void addShader(unsigned handle);
    void freeShader(int id);
    void link();
    void bind();
    int getUniformLocation(HashString name);
    void setUniform(int loc, const glm::vec2& v);
    void setUniform(int loc, const glm::vec3& v);
    void setUniform(int loc, const glm::vec4& v);
    void setUniform(int loc, const glm::mat3& v);
    void setUniform(int loc, const glm::mat4& v);
    void setUniformInt(int loc, const int v);
    void setUniformFloat(int loc, const float v);
    void bindAlbedo(int channel);
    void bindNormal(int channel);
};
