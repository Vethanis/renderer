#include "glscreen.h"
#include "myglheaders.h"
#include "debugmacro.h"
#include "shader.h"

namespace GLScreen 
{
    unsigned vertHandle;
    unsigned vao, vbo;

    void init()
    {
        glGenVertexArrays(1, &vao); DebugGL();
        glGenBuffers(1, &vbo); DebugGL();
        glBindVertexArray(vao); DebugGL();
        glBindBuffer(GL_ARRAY_BUFFER, vbo); DebugGL();
        static const GLfloat coords[] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            1.0f,  1.0f,
            1.0f,  1.0f,
            -1.0f,  1.0f,
            -1.0f, -1.0f
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(coords[0]) * 12, coords, GL_STATIC_DRAW); DebugGL();
        glEnableVertexAttribArray(0); DebugGL();
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0); DebugGL();
        glBindBuffer(GL_ARRAY_BUFFER, 0); DebugGL();
    }

    void draw()
    {
        glBindVertexArray(vao); DebugGL();
        glBindBuffer(GL_ARRAY_BUFFER, vbo); DebugGL();
        glDrawArrays(GL_TRIANGLES, 0, 6); DebugGL();
    }

}; // namespace GLScreen
