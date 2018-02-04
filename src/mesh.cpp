
#define _CRT_SECURE_NO_WARNINGS

#include "myglheaders.h"
#include "mesh.h"
#include "debugmacro.h"
#include "shared_uniform.h"

template<typename T>
struct mesh_layout
{
    size_t offset_loc;
    size_t stride;

    mesh_layout()
    {
        offset_loc = 0;
        stride = sizeof(T);
    }

    template<typename T>
    void layout(int location);

    template<>
    void layout<glm::vec2>(int location)
    {
        glEnableVertexAttribArray(location); DebugGL();;
        glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, (int)stride, (void*)offset_loc); DebugGL();;
        offset_loc += sizeof(glm::vec2);
    }
    template<>
    void layout<glm::vec3>(int location)
    {
        glEnableVertexAttribArray(location); DebugGL();;
        glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, (int)stride, (void*)offset_loc); DebugGL();;
        offset_loc += sizeof(glm::vec3);
    }
    template<>
    void layout<glm::vec4>(int location)
    {
        glEnableVertexAttribArray(location); DebugGL();;
        glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, (int)stride, (void*)offset_loc); DebugGL();;
        offset_loc += sizeof(glm::vec4);
    }
    template<>
    void layout<unsigned>(int location)
    {
        glEnableVertexAttribArray(location); DebugGL();
        glVertexAttribPointer(location, 1, GL_UNSIGNED_INT, GL_FALSE, (int)stride, (void*)offset_loc); DebugGL();
        offset_loc += sizeof(unsigned);
    }
};

void Mesh::init()
{
    num_indices = 0;
    glGenVertexArrays(1, &vao); DebugGL();;
    glGenBuffers(1, &vbo); DebugGL();;

    glBindVertexArray(vao); DebugGL();;
    glBindBuffer(GL_ARRAY_BUFFER, vbo); DebugGL();;

    mesh_layout<Vertex> ml;
    ml.layout<glm::vec3>(0);    // pos
}

void Mesh::deinit()
{
    glDeleteBuffers(1, &vbo); DebugGL();;
    glDeleteVertexArrays(1, &vao); DebugGL();;
    DebugGL();
}

void Mesh::upload(const Vertex* p, const u32 count)
{
    glBindVertexArray(vao); DebugGL();

    glBindBuffer(GL_ARRAY_BUFFER, vbo); DebugGL();
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * count, 
        p, GL_STATIC_DRAW); DebugGL();
        
    num_indices = count;
}

void Mesh::draw()const
{
    if(!num_indices)
        return;

    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); DebugGL();

    NotifySharedUniformsUpdated();

    glBindVertexArray(vao); DebugGL();;
    glDrawArrays(GL_TRIANGLES, 0, num_indices);
    
    //glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); DebugGL();
}