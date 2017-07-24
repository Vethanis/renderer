#include "myglheaders.h"

#include "glprogram.h"

#include "shader.h"
#include "loadfile.h"
#include "debugmacro.h"
#include "glm/gtc/type_ptr.hpp"
#include "stdio.h"
#include "filestore.h"

void GLProgram::init(){
    shader_tail = 0;
    id = glCreateProgram();  MYGLERRORMACRO;
}

void GLProgram::deinit(){
    glDeleteProgram(id); MYGLERRORMACRO;
}

void GLProgram::addShader(const char* path, int type){
    char* src = load_file(path);
    unsigned handle = createShader(src, type);
    glAttachShader(id, handle);  MYGLERRORMACRO;
    release_file(src);
    shader_handles[shader_tail++] = handle;
    assert(shader_tail < max_shaders);
}

void GLProgram::addShader(unsigned handle){
    glAttachShader(id, handle);  MYGLERRORMACRO;
    // don't put in shader_handles as we don't own this shader.
}

void GLProgram::link(){
    glLinkProgram(id);  MYGLERRORMACRO;

    int result = 0;
    glGetProgramiv(id, GL_LINK_STATUS, &result);
    if(!result){
        int loglen = 0;
        glGetProgramiv(id, GL_INFO_LOG_LENGTH, &loglen);  MYGLERRORMACRO;
        char* log = new char[loglen + 1];
        glGetProgramInfoLog(id, loglen, NULL, log);  MYGLERRORMACRO;
        log[loglen] = 0;
        puts(log);
        delete[] log;
    }

    for(unsigned i = 0; i < shader_tail; i++){
        deleteShader(shader_handles[i]);  MYGLERRORMACRO;
    }
    shader_tail = 0;
}

void GLProgram::bind(){
    glUseProgram(id);  MYGLERRORMACRO;
}

int GLProgram::getUniformLocation(unsigned name){
    int* loc = m_locations[name];
    if(!loc){
        const char* strName = g_nameStore[name];
        m_locations.insert(name, glGetUniformLocation(id, strName));  MYGLERRORMACRO;
        loc = m_locations[name];
    }
    return *loc;
}

void GLProgram::setUniform(unsigned name, const glm::vec2& v){
    const int location = getUniformLocation(name); 
    glUniform2fv(location, 1, glm::value_ptr(v));  MYGLERRORMACRO;
}
void GLProgram::setUniform(unsigned name, const glm::vec3& v){
    const int location = getUniformLocation(name);
    glUniform3fv(location, 1, glm::value_ptr(v));  MYGLERRORMACRO;
}
void GLProgram::setUniform(unsigned name, const glm::vec4& v){
    const int location = getUniformLocation(name);
    glUniform4fv(location, 1, glm::value_ptr(v));  MYGLERRORMACRO;
}
void GLProgram::setUniform(unsigned name, const glm::mat3& v){
    const int location = getUniformLocation(name);
    glUniformMatrix3fv(location, 1, false, glm::value_ptr(v));  MYGLERRORMACRO;
}
void GLProgram::setUniform(unsigned name, const glm::mat4& v){
    const int location = getUniformLocation(name);
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(v));  MYGLERRORMACRO;
}
void GLProgram::setUniformInt(unsigned name, const int v){
    const int location = getUniformLocation(name);
    glUniform1i(location, v);  MYGLERRORMACRO;
}
void GLProgram::setUniformFloat(unsigned name, const float v){
    const int location = getUniformLocation(name);
    glUniform1f(location, v);  MYGLERRORMACRO;
}
