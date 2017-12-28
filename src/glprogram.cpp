#include "myglheaders.h"

#include "glprogram.h"

#include "shader.h"
#include "loadfile.h"
#include "debugmacro.h"
#include "glm/gtc/type_ptr.hpp"
#include "stdio.h"

void GLProgram::init(){
    m_id = glCreateProgram();  DebugGL();;
}

void GLProgram::deinit(){
    glDeleteProgram(m_id); DebugGL();;
}

s32 GLProgram::addShader(const char* path, s32 type){
    char* src = load_file(path);
    u32 handle = createShader(src, type);
    glAttachShader(m_id, handle);  DebugGL();;
    release_file(src);
    return handle;
}

void GLProgram::addShader(u32 handle){
    glAttachShader(m_id, handle);  DebugGL();;
}

void GLProgram::freeShader(u32 handle){
    glDetachShader(m_id, handle);  DebugGL();;
    glDeleteShader(handle);  DebugGL();;
}

void GLProgram::link(){
    glLinkProgram(m_id);  DebugGL();;

    s32 result = 0;
    glGetProgramiv(m_id, GL_LINK_STATUS, &result);
    if(!result){
        s32 loglen = 0;
        glGetProgramiv(m_id, GL_INFO_LOG_LENGTH, &loglen);  DebugGL();;
        char* log = new char[loglen + 1];
        glGetProgramInfoLog(m_id, loglen, nullptr, log);  DebugGL();;
        log[loglen] = 0;
        puts(log);
        delete[] log;
    }
}

void GLProgram::bind(){
    glUseProgram(m_id);  DebugGL();;
}

s32 GLProgram::getUniformLocation(HashString hash){
    s32* pLoc = locations[hash.m_hash];
    if(!pLoc){
        const char* loc_str = hash.str();
        Assert(loc_str);
        locations.insert(hash.m_hash, glGetUniformLocation(m_id, loc_str));
        pLoc = locations[hash.m_hash];
        if(*pLoc == -1){
            printf("[GLProgram] Invalid uniform detected: %s\n", loc_str);
            //Assert(false);
        }
    }
    return *pLoc;
}

void GLProgram::setUniform(s32 location, const glm::vec2& v){
    glUniform2fv(location, 1, glm::value_ptr(v));  DebugGL();;
}
void GLProgram::setUniform(s32 location, const glm::vec3& v){
    glUniform3fv(location, 1, glm::value_ptr(v));  DebugGL();;
}
void GLProgram::setUniform(s32 location, const glm::vec4& v){
    glUniform4fv(location, 1, glm::value_ptr(v));  DebugGL();;
}
void GLProgram::setUniform(s32 location, const glm::mat3& v){
    glUniformMatrix3fv(location, 1, false, glm::value_ptr(v));  DebugGL();;
}
void GLProgram::setUniform(s32 location, const glm::mat4& v){
    glUniformMatrix4fv(location, 1, false, glm::value_ptr(v));  DebugGL();;
}
void GLProgram::setUniformInt(s32 location, s32 v){
    glUniform1i(location, v);  DebugGL();;
}
void GLProgram::setUniformFloat(s32 location, float v){
    glUniform1f(location, v);  DebugGL();;
}

void GLProgram::computeCall(s32 x, s32 y, s32 z){
    glDispatchCompute(x, y, z);
}

void GLProgram::setup(const char** filenames, s32 count){
    static const s32 sequences[] = {
        GL_COMPUTE_SHADER, 
        GL_VERTEX_SHADER, 
        GL_FRAGMENT_SHADER, 
        GL_VERTEX_SHADER, 
        GL_GEOMETRY_SHADER,
        GL_FRAGMENT_SHADER
    };
    Assert(count > 0 && count < 4);
    s32 names[] = {0, 0, 0};
    init();
    s32 begin = 0;
    if(count == 2){
        begin = 1;
    }
    else if(count == 3){
        begin = 3;
    }
    for(s32 i = 0; i < count; ++i){
        names[i] = addShader(filenames[i], sequences[begin + i]);
    }
    link();
    for(s32 i = 0; i < count; ++i){
        freeShader(names[i]);
    }
}

void GLProgram::bindTexture(s32 channel, s32 texture, const char* name)
{
    glActiveTexture(GL_TEXTURE0 + channel);  DebugGL();
    glBindTexture(GL_TEXTURE_2D, texture);  DebugGL();
    setUniformInt(name, channel);
}

void GLProgram::bindCubemap(s32 channel, s32 texture, const char* name)
{
    glActiveTexture(GL_TEXTURE0 + channel);  DebugGL();
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);  DebugGL();
    setUniformInt(name, channel);
}