
#define _CRT_SECURE_NO_WARNINGS

#include "myglheaders.h"
#include "mesh.h"
#include "debugmacro.h"

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
    glGenBuffers(1, &ebo); DebugGL();

    glBindVertexArray(vao); DebugGL();;
    glBindBuffer(GL_ARRAY_BUFFER, vbo); DebugGL();;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); DebugGL();

    mesh_layout<Vertex> ml;
    ml.layout<glm::vec3>(0); // pos
    ml.layout<unsigned>(1); // normal
    ml.layout<unsigned>(2); // color
    ml.layout<unsigned>(3); // material
}

void Mesh::deinit()
{
    glDeleteBuffers(1, &ebo); DebugGL();
    glDeleteBuffers(1, &vbo); DebugGL();;
    glDeleteVertexArrays(1, &vao); DebugGL();;
    DebugGL();
}

void Mesh::upload(const Geometry& geom)
{
    glBindVertexArray(vao); DebugGL();

    glBindBuffer(GL_ARRAY_BUFFER, vbo); DebugGL();
    glBufferData(GL_ARRAY_BUFFER, geom.vertices.bytes(), 
        geom.vertices.begin(), GL_STATIC_DRAW); DebugGL();
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); DebugGL();
    num_indices = geom.indices.count();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, geom.indices.bytes(), 
        geom.indices.begin(), GL_STATIC_DRAW); DebugGL();
}

void Mesh::draw()const
{
    if(!num_indices)
        return;

    glBindVertexArray(vao); DebugGL();
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0); DebugGL();
}