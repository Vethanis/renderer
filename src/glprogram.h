#pragma once

#include "ints.h"
#include "glm/glm.hpp"
#include "store.h"
#include "hashstring.h"

struct GLProgram{
    u32 m_id;
    Store<s32, 8> locations;
    void init();
    void deinit();
    s32 addShader(const char* path, s32 type);
    void addShader(u32 handle);
    void freeShader(u32 handle);
    void link();
    void bind();
    s32 getUniformLocation(HashString name);
    void setUniform(s32 loc, const glm::vec2& v);
    void setUniform(s32 loc, const glm::vec3& v);
    void setUniform(s32 loc, const glm::vec4& v);
    void setUniform(s32 loc, const glm::mat3& v);
    void setUniform(s32 loc, const glm::mat4& v);
    void setUniformInt(s32 loc, s32 v);
    void setUniformFloat(s32 loc, float v);
    void bindTexture(s32 channel, s32 texture, const char* name);
    void bindCubemap(s32 channel, s32 texture, const char* name);
    void bind3DTexture(s32 channel, s32 texture, const char* name);
    void computeCall(s32 x=0, s32 y=0, s32 z=0);
    void setup(const char** filenames, s32 count);
    template<typename T>
    void setUniform(const char* name, const T& t){
        s32 loc = getUniformLocation(HashString(name));
        setUniform(loc, t);
    }
    void setUniformInt(const char* name, s32 v){
        s32 loc = getUniformLocation(HashString(name));
        setUniformInt(loc, v);
    }
    void setUniformFloat(const char* name, float v){
        s32 loc = getUniformLocation(HashString(name));
        setUniformFloat(loc, v);
    }
};
