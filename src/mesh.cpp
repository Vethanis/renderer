
#define _CRT_SECURE_NO_WARNINGS

#include "myglheaders.h"
#include "mesh.h"
#include "debugmacro.h"
#include "vertexbuffer.h"
#include "glm/glm.hpp"
#include "loadfile.h"
#include "hashstring.h"

template<typename T>
void mesh_layout(int location);

static size_t offset_loc = 0;
static size_t stride = 0;

template<typename T>
void begin_mesh_layout(){
    offset_loc = 0;
    stride = sizeof(T);
}

template<>
inline void mesh_layout<glm::vec2>(int location){
    glEnableVertexAttribArray(location); DebugGL();;
    glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, (int)stride, (void*)offset_loc); DebugGL();;
    offset_loc += sizeof(glm::vec2);
}
template<>
inline void mesh_layout<glm::vec3>(int location){
    glEnableVertexAttribArray(location); DebugGL();;
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, (int)stride, (void*)offset_loc); DebugGL();;
    offset_loc += sizeof(glm::vec3);
}
template<>
inline void mesh_layout<glm::vec4>(int location){
    glEnableVertexAttribArray(location); DebugGL();;
    glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, (int)stride, (void*)offset_loc); DebugGL();;
    offset_loc += sizeof(glm::vec4);
}
template<>
inline void mesh_layout<unsigned>(int location){
    glEnableVertexAttribArray(location); DebugGL();
    glVertexAttribPointer(location, 1, GL_UNSIGNED_INT, GL_FALSE, (int)stride, (void*)offset_loc); DebugGL();
    offset_loc += sizeof(unsigned);
}

void Mesh::init(){
    num_indices = 0;
    glGenVertexArrays(1, &vao); DebugGL();;
    glGenBuffers(1, &vbo); DebugGL();;
    glGenBuffers(1, &ebo); DebugGL();

    glBindVertexArray(vao); DebugGL();;
    glBindBuffer(GL_ARRAY_BUFFER, vbo); DebugGL();;
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); DebugGL();

    begin_mesh_layout<Vertex>();
    mesh_layout<glm::vec4>(0);
    mesh_layout<glm::vec4>(1);
    mesh_layout<glm::vec3>(2);
    mesh_layout<unsigned>(3);
}

void Mesh::deinit(){
    glDeleteBuffers(1, &ebo); DebugGL();
    glDeleteBuffers(1, &vbo); DebugGL();;
    glDeleteVertexArrays(1, &vao); DebugGL();;
    DebugGL();
}

void Mesh::upload(const mesh_interchange::Model& vb){
    glBindVertexArray(vao); DebugGL();

    glBindBuffer(GL_ARRAY_BUFFER, vbo); DebugGL();
    glBufferData(GL_ARRAY_BUFFER, vb.meshes.vertices.bytes(), 
        vb.meshes.vertices.begin(), GL_STATIC_DRAW); DebugGL();
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo); DebugGL();
    num_indices = vb.meshes.indices.count();
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, vb.meshes.indices.bytes(), 
        vb.meshes.indices.begin(), GL_STATIC_DRAW); DebugGL();
}

void Mesh::draw(){
    if(!num_indices)
        return;

    glBindVertexArray(vao); DebugGL();
    glDrawElements(GL_TRIANGLES, num_indices, GL_UNSIGNED_INT, 0); DebugGL();
}

void MeshStore::load_mesh(Mesh& mesh, HashString name){
    auto* vb = m_vbs[name.m_hash];

    if(vb){
        mesh.upload(*vb);
        return;
    }

    if(m_vbs.full()){
        vb = m_vbs.reuse_near(name.m_hash);
    }
    else{
        m_vbs.insert(name.m_hash, {});
        vb = m_vbs[name.m_hash];
    }

    const char* filename = name;
    assert(filename);
    FILE* pFile = fopen(filename, "rb");

    if(pFile){
        vb->load(pFile);
        fclose(pFile);
    }
    else{        
        char buff[256] = {0};
    
        sprintf(buff, "%s", filename);
        char* extptr = strstr(buff, ".mesh");
        assert(extptr);
        sprintf(extptr, "%s", ".fbx");

        vb->parse(buff);
        
        pFile = fopen(filename, "wb");
        assert(pFile);
        vb->serialize(pFile);
        fclose(pFile);
    }

    mesh.upload(*vb);

    printf("[mesh] loaded %s\n", filename);
}

MeshStore g_MeshStore;

HashString::operator Mesh*() const{
    return g_MeshStore[m_hash];
}