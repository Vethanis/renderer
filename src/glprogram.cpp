#include "myglheaders.h"

#include "glprogram.h"

#include "shader.h"
#include "loadfile.h"
#include "debugmacro.h"
#include "glm/gtc/type_ptr.hpp"
#include "stdio.h"
#include "filestore.h"

void GLProgram::init(){
    id = glCreateProgram();  MYGLERRORMACRO;
}

void GLProgram::deinit(){
    glDeleteProgram(id); MYGLERRORMACRO;
}

int GLProgram::addShader(const char* path, int type){
    char* src = load_file(path);
    unsigned handle = createShader(src, type);
    glAttachShader(id, handle);  MYGLERRORMACRO;
    release_file(src);
    return id;
}

void GLProgram::addShader(unsigned handle){
    glAttachShader(id, handle);  MYGLERRORMACRO;
}


void GLProgram::freeShader(int id){
    deleteShader(id);
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
}

void GLProgram::bind(){
    glUseProgram(id);  MYGLERRORMACRO;
}

int GLProgram::getUniformLocation(const char* name){
    return glGetUniformLocation(id, name);
}

void GLProgram::setUniform(int location, const glm::vec2& v){
    glUniform2fv(location, 1, glm::value_ptr(v));  MYGLERRORMACRO;
}
void GLProgram::setUniform(int location, const glm::vec3& v){
    glUniform3fv(location, 1, glm::value_ptr(v));  MYGLERRORMACRO;
}
void GLProgram::setUniform(int location, const glm::vec4& v){
    glUniform4fv(location, 1, glm::value_ptr(v));  MYGLERRORMACRO;
}
void GLProgram::setUniform(int location, const glm::mat3& v){
    glUniformMatrix3fv(location, 1, false, glm::value_ptr(v));  MYGLERRORMACRO;
}
void GLProgram::setUniform(int location, const glm::mat4& v){
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(v));  MYGLERRORMACRO;
}
void GLProgram::setUniformInt(int location, const int v){
    glUniform1i(location, v);  MYGLERRORMACRO;
}
void GLProgram::setUniformFloat(int location, const float v){
    glUniform1f(location, v);  MYGLERRORMACRO;
}
