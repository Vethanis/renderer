#include "myglheaders.h"
#include "SSBO.h"
#include "debugmacro.h"
#include "string.h"

void SSBO::init(unsigned binding){
    glGenBuffers(1, &id);
    MYGLERRORMACRO
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, binding, id);
    MYGLERRORMACRO
}
void SSBO::deinit(){
    glDeleteBuffers(1, &id);
    MYGLERRORMACRO
}
void SSBO::upload(const void* ptr, unsigned bytes){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    MYGLERRORMACRO
    glBufferData(GL_SHADER_STORAGE_BUFFER, bytes, ptr, GL_DYNAMIC_COPY);
    MYGLERRORMACRO
}
void SSBO::download(void* dest, unsigned bytes){
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, id);
    MYGLERRORMACRO
    void* src = glMapNamedBuffer(id, GL_DYNAMIC_COPY);
    memcpy(dest, src, bytes);
    glUnmapNamedBuffer(id);
    MYGLERRORMACRO
}