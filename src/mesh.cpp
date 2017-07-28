
#define _CRT_SECURE_NO_WARNINGS

#include "myglheaders.h"
#include "mesh.h"
#include "debugmacro.h"
#include "vertexbuffer.h"
#include "glm/glm.hpp"
#include "filestore.h"

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
inline void mesh_layout<int>(int location){
    glEnableVertexAttribArray(location); MYGLERRORMACRO;
    glVertexAttribPointer(location, 1, GL_INT, GL_FALSE, (int)stride, (void*)offset_loc); MYGLERRORMACRO;
    offset_loc += sizeof(int);
}
template<>
inline void mesh_layout<float>(int location){
    glEnableVertexAttribArray(location); MYGLERRORMACRO;
    glVertexAttribPointer(location, 1, GL_FLOAT, GL_FALSE, (int)stride, (void*)offset_loc); MYGLERRORMACRO;
    offset_loc += sizeof(float);
}
template<>
inline void mesh_layout<glm::vec2>(int location){
    glEnableVertexAttribArray(location); MYGLERRORMACRO;
    glVertexAttribPointer(location, 2, GL_FLOAT, GL_FALSE, (int)stride, (void*)offset_loc); MYGLERRORMACRO;
    offset_loc += sizeof(glm::vec2);
}
template<>
inline void mesh_layout<glm::vec3>(int location){
    glEnableVertexAttribArray(location); MYGLERRORMACRO;
    glVertexAttribPointer(location, 3, GL_FLOAT, GL_FALSE, (int)stride, (void*)offset_loc); MYGLERRORMACRO;
    offset_loc += sizeof(glm::vec3);
}
template<>
inline void mesh_layout<glm::vec4>(int location){
    glEnableVertexAttribArray(location); MYGLERRORMACRO;
    glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, (int)stride, (void*)offset_loc); MYGLERRORMACRO;
    offset_loc += sizeof(glm::vec4);
}

void Mesh::init(){
    num_vertices = 0;
    glGenVertexArrays(1, &vao); MYGLERRORMACRO;
    glGenBuffers(1, &vbo); MYGLERRORMACRO;

    glBindVertexArray(vao); MYGLERRORMACRO;
    glBindBuffer(GL_ARRAY_BUFFER, vbo); MYGLERRORMACRO;

    begin_mesh_layout<Vertex>();
    mesh_layout<glm::vec4>(0);
    mesh_layout<glm::vec4>(1);
    mesh_layout<glm::vec4>(2);
    mesh_layout<glm::vec4>(3);

}

void Mesh::deinit(){
    glDeleteBuffers(1, &vbo); MYGLERRORMACRO;
    glDeleteVertexArrays(1, &vao); MYGLERRORMACRO;
    MYGLERRORMACRO
}

void Mesh::upload(const VertexBuffer& vb){
    glBindVertexArray(vao); MYGLERRORMACRO;
    glBindBuffer(GL_ARRAY_BUFFER, vbo); MYGLERRORMACRO;
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vb.size(), vb.data(), GL_STATIC_DRAW); MYGLERRORMACRO;
    num_vertices = unsigned(vb.size());
}

void Mesh::draw(){
    if(!num_vertices){return;}
    glBindVertexArray(vao); MYGLERRORMACRO;
    glDrawArrays(GL_TRIANGLES, 0, num_vertices); MYGLERRORMACRO;
}

void parse_obj(VertexBuffer& out, const char* text){
    const char* p = text;

    struct Face{
        int v1, vt1, vn1, v2, vt2, vn2, v3, vt3, vn3;
    };

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    std::vector<Face> faces;

    while(*p && p[1]){
        switch(*p){
            case 'v':
            {
                switch(p[1]){
                    case ' ':
                    {
                        positions.push_back({});
                        glm::vec3& pos = positions.back();
                        sscanf(p, "v %f %f %f", &pos.x, &pos.y, &pos.z);
                    }
                    break;
                    case 't':
                    {
                        uvs.push_back({});
                        glm::vec2& uv = uvs.back();
                        sscanf(p, "vt %f %f", &uv.x, &uv.y);
                    }
                    break;
                    case 'n':
                    {
                        normals.push_back({});
                        glm::vec3& norm = normals.back();
                        sscanf(p, "vn %f %f %f", &norm.x, &norm.y, &norm.z);
                        norm = glm::normalize(norm);
                    }
                    break;
                }
            }
            break;
            case 'f':
            {
                faces.push_back({});
                Face& f = faces.back();
                sscanf(p, "f %i/%i/%i %i/%i/%i %i/%i/%i", &f.v1, &f.vt1, &f.vn1,
                    &f.v2, &f.vt2, &f.vn2,
                    &f.v3, &f.vt3, &f.vn3);
                
            }
        }
        p = nextline(p);
    }

    out.clear();
    for(Face& face : faces){
        glm::vec3& pa = positions[face.v1 - 1];
        glm::vec3& pb = positions[face.v2 - 1];
        glm::vec3& pc = positions[face.v3 - 1];

        glm::vec3& na = normals[face.vn1 - 1];
        glm::vec3& nb = normals[face.vn2 - 1];
        glm::vec3& nc = normals[face.vn3 - 1];

        glm::vec2& ua = uvs[face.vt1 - 1];
        glm::vec2& ub = uvs[face.vt2 - 1];
        glm::vec2& uc = uvs[face.vt3 - 1];

        glm::vec3 e1 = pb - pa;
        glm::vec3 e2 = pc - pa;
        glm::vec2 duv1 = ub - ua;
        glm::vec2 duv2 = uc - ua;

        glm::vec3 t, b;
        float f = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);

        t.x = f * (duv2.y * e1.x - duv1.y * e2.x);
        t.y = f * (duv2.y * e1.y - duv1.y * e2.y);
        t.z = f * (duv2.y * e1.z - duv1.y * e2.z);
        t = glm::normalize(t);

        b.x = f * (-duv2.x * e1.x + duv1.x * e2.x);
        b.y = f * (-duv2.x * e1.y + duv1.x * e2.y);
        b.z = f * (-duv2.x * e1.z + duv1.x * e2.z);
        b = glm::normalize(b);

        int mat = 0;

        out.push_back({
            glm::vec4(pa.x, pa.y, pa.z, ua.x), 
            glm::vec4(na.x, na.y, na.z, ua.y), 
            glm::vec4(t.x, t.y, t.z, float(mat)), 
            glm::vec4(b.x, b.y, b.z, 0.0f)
        });
        out.push_back({
            glm::vec4(pb.x, pb.y, pb.z, ub.x), 
            glm::vec4(nb.x, nb.y, nb.z, ub.y), 
            glm::vec4(t.x, t.y, t.z, float(mat)), 
            glm::vec4(b.x, b.y, b.z, 0.0f)
        });
        out.push_back({
            glm::vec4(pc.x, pc.y, pc.z, uc.x), 
            glm::vec4(nc.x, nc.y, nc.z, uc.y), 
            glm::vec4(t.x, t.y, t.z, float(mat)), 
            glm::vec4(b.x, b.y, b.z, 0.0f)
        });
    }
}

void MeshStore::load_mesh(Mesh& mesh, unsigned name){
    const char* filename = g_nameStore.get(name);
    assert(filename);
    const char* contents = load_file(filename);
    parse_obj(vb, contents); 
    release_file(contents);
    mesh.upload(vb);
}

MeshStore g_MeshStore;