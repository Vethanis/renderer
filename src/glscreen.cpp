#include "glscreen.h"
#include "myglheaders.h"
#include "debugmacro.h"
#include "shader.h"

static const char* shader_text = "\n\
#version 330 core\n\
layout(location = 0) in vec2 uv;\n\
out vec2 fragUv;\n\
void main(){\n\
    gl_Position = vec4(uv, 0.0, 1.0);\n\
    fragUv = uv * 0.5 + 0.5;\n\
}\n\
";

static unsigned vertHandle = 0;

void GLScreen::init(){
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    static const GLfloat coords[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f,
        -1.0f, -1.0f
    };
    glBufferData(GL_ARRAY_BUFFER, sizeof(coords[0]) * 12, coords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    MYGLERRORMACRO
    if(!vertHandle){
        vertHandle = createShader(shader_text, GL_VERTEX_SHADER);
    }
    vertexShader = vertHandle;
}

void GLScreen::deinit(){
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    deleteShader(vertexShader);
    MYGLERRORMACRO
}

void GLScreen::draw(){
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    MYGLERRORMACRO
}

