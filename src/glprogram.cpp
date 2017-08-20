#include "myglheaders.h"

#include "glprogram.h"

#include "shader.h"
#include "loadfile.h"
#include "debugmacro.h"
#include "glm/gtc/type_ptr.hpp"
#include "stdio.h"
#include "namestore.h"

void GLProgram::init(){
    m_id = glCreateProgram();  DebugGL();;
}

void GLProgram::deinit(){
    glDeleteProgram(m_id); DebugGL();;
}

int GLProgram::addShader(const char* path, int type){
    char* src = load_file(path);
    unsigned handle = createShader(src, type);
    glAttachShader(m_id, handle);  DebugGL();;
    release_file(src);
    return handle;
}

void GLProgram::addShader(unsigned handle){
    glAttachShader(m_id, handle);  DebugGL();;
}


void GLProgram::freeShader(unsigned handle){
    deleteShader(handle);
}

void GLProgram::link(){
    glLinkProgram(m_id);  DebugGL();;

    int result = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &result);
    if(!result){
        int loglen = 0;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &loglen);  DebugGL();;
        char* log = new char[loglen + 1];
        glGetProgramInfoLog(m_id, loglen, NULL, log);  DebugGL();;
        log[loglen] = 0;
        puts(log);
        delete[] log;
    }
}

void GLProgram::bind(){
    glUseProgram(m_id);  DebugGL();;
}

int GLProgram::getUniformLocation(HashString hash){
    int* pLoc = locations[hash.m_hash];
    if(!pLoc){
        locations.insert(hash.m_hash, glGetUniformLocation(m_id, hash));
        pLoc = locations[hash.m_hash];
        if(*pLoc == -1){
            printf("[GLProgram] Invalid uniform detected: %s\n", hash.str());
            assert(false);
        }
    }
    return *pLoc;
}

void GLProgram::setUniform(int location, const glm::vec2& v){
    glUniform2fv(location, 1, glm::value_ptr(v));  DebugGL();;
}
void GLProgram::setUniform(int location, const glm::vec3& v){
    glUniform3fv(location, 1, glm::value_ptr(v));  DebugGL();;
}
void GLProgram::setUniform(int location, const glm::vec4& v){
    glUniform4fv(location, 1, glm::value_ptr(v));  DebugGL();;
}
void GLProgram::setUniform(int location, const glm::mat3& v){
    glUniformMatrix3fv(location, 1, false, glm::value_ptr(v));  DebugGL();;
}
void GLProgram::setUniform(int location, const glm::mat4& v){
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(v));  DebugGL();;
}
void GLProgram::setUniformInt(int location, const int v){
    glUniform1i(location, v);  DebugGL();;
}
void GLProgram::setUniformFloat(int location, const float v){
    glUniform1f(location, v);  DebugGL();;
}

static const unsigned albedo_names[] = {
    HashString("albedoSampler0"),
    HashString("albedoSampler1"),
    HashString("albedoSampler2"),
    HashString("albedoSampler3"),
};
static const unsigned normal_names[] = {
    HashString("normalSampler0"),
    HashString("normalSampler1"),
    HashString("normalSampler2"),
    HashString("normalSampler3"),
};

void GLProgram::bindAlbedo(int channel){
    unsigned loc = getUniformLocation(albedo_names[channel]);
    setUniformInt(loc, channel * 2);
}
void GLProgram::bindNormal(int channel){
    unsigned loc = getUniformLocation(normal_names[channel]);
    setUniformInt(loc, channel * 2 + 1);
}
void GLProgram::computeCall(int x, int y, int z){
    glDispatchCompute(x, y, z);
}