
#define _CRT_SECURE_NO_WARNINGS

#include "myglheaders.h"
#include "mesh.h"
#include "debugmacro.h"
#include "vertexbuffer.h"
#include "glm/glm.hpp"
#include <glm/gtx/common.hpp>
#include "filestore.h"

template<typename T>
void mesh_layout(int location);

static int offset_loc = 0;
static int stride = 0;

template<typename T>
void begin_mesh_layout(){
    offset_loc = 0;
    stride = sizeof(T);
}

template<>
inline void mesh_layout<float>(int location){
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 1, GL_FLOAT, GL_FALSE, stride, (void*)offset_loc);
    offset_loc += sizeof(float);
}
template<>
inline void mesh_layout<glm::vec2>(int location){
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, stride, (void*)offset_loc);
    offset_loc += sizeof(glm::vec2);
}
template<>
inline void mesh_layout<glm::vec3>(int location){
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, stride, (void*)offset_loc);
    offset_loc += sizeof(glm::vec3);
}
template<>
inline void mesh_layout<glm::vec4>(int location){
    glEnableVertexAttribArray(location);
    glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, stride, (void*)offset_loc);
    offset_loc += sizeof(glm::vec4);
}

void Mesh::init(){
    num_vertices = 0;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);

    begin_mesh_layout<Vertex>();
    mesh_layout<glm::vec3>(0);
    mesh_layout<glm::vec3>(1);
    mesh_layout<glm::vec2>(2);

    MYGLERRORMACRO
}

void Mesh::deinit(){
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    MYGLERRORMACRO
}

void Mesh::upload(const VertexBuffer& vb){
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vb.size(), vb.data(), GL_STATIC_DRAW);
    MYGLERRORMACRO
    num_vertices = unsigned(vb.size());
}

void Mesh::draw(){
    if(!num_vertices){return;}
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, num_vertices);
    MYGLERRORMACRO
}

void parse_mesh_file(VertexBuffer& out, const char* text){
    const char* p = text;
    int num_verts = 0, num_faces = 0;
    bool in_header = true;

    while(in_header && p[0]){
        if(strncmp("end_header", p, 10) == 0){
            in_header = false;
        }
        else if(strncmp("element vertex ", p, 10) == 0){
            sscanf(p, "%*s %*s %i\n", &num_verts);
        }
        else if(strncmp("element face ", p, 10) == 0){
            sscanf(p, "%*s %*s %i\n", &num_faces);
        }
        p = nextline(p);
    }

    VertexBuffer verts;
    out.clear();
    out.resize(num_faces * 3);

    for(int i = 0; i < num_verts && p[0]; i++){
        Vertex v;
        glm::ivec3 c;
        sscanf(p, "%f %f %f %f %f %f %f %f\n", &v.position.x, &v.position.y, &v.position.z, &v.normal.x, &v.normal.y, &v.normal.z, &v.uv.x, &v.uv.y);
        v.normal = glm::normalize(v.normal);
        v.uv = glm::fmod(v.uv, glm::vec2(1.0f));
        verts.push_back(v);
        p = nextline(p);
    }

    for(int i = 0; i < num_faces && p[0]; i++){
        int a, b, c;
        sscanf(p, "%*i %i %i %i\n", &a, &b, &c);
        out.push_back(verts[a]);
        out.push_back(verts[b]);
        out.push_back(verts[c]);
        p = nextline(p);
    }
}

void MeshStore::load_mesh(Mesh& mesh, unsigned name){
    const char* filename = g_nameStore.get(name);
    assert(filename);
    const char* contents = load_file(filename);
    parse_mesh_file(vb, contents); 
    mesh.upload(vb);
    release_file(contents);
}

MeshStore g_MeshStore;