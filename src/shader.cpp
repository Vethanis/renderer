#include "shader.h"

#include "myglheaders.h"
#include "stdio.h"

unsigned createShader(const char* src, int type){
    unsigned handle = glCreateShader(type);  MYGLERRORMACRO;
    glShaderSource(handle, 1, &src, NULL);  MYGLERRORMACRO;
    glCompileShader(handle);  MYGLERRORMACRO;

    int result = 0;
    glGetShaderiv(handle, GL_COMPILE_STATUS, &result);  MYGLERRORMACRO;

    if(!result){
        int loglen = 0;
        glGetShaderiv(handle, GL_INFO_LOG_LENGTH, &loglen);  MYGLERRORMACRO;
        char* log = new char[loglen + 1];
        glGetShaderInfoLog(handle, loglen, NULL, log);  MYGLERRORMACRO;
        log[loglen] = 0;
        puts(log);
        delete[] log;
    }

    return handle;
}

void deleteShader(unsigned id){
    glDeleteShader(id);  MYGLERRORMACRO;
}
