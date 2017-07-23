#pragma once

#include "store.h"
#include "vertexbuffer.h"

struct Mesh {
    unsigned vao, vbo, num_vertices;
    void draw();
    void upload(const VertexBuffer& vb);
    void init();
    void deinit();
};

struct MeshStore{
    Store<Mesh, 128> m_store;
    VertexBuffer vb;
    
    void load_mesh(Mesh& mesh, unsigned name);
    Mesh* get(unsigned name){
        Mesh* m = m_store.get(name);
        if(m){ return m; }

        if(m_store.full()){
            m_store.remove_near(name)->deinit();
        }

        Mesh new_mesh;
        new_mesh.init();
        load_mesh(new_mesh, name);
        m_store.insert(name, new_mesh);
        return m_store.get(name);
    }
    Mesh* operator[](unsigned name){ return get(name); }
    Mesh* operator[](const char* name){ return get(hash(name)); }
};

extern MeshStore g_MeshStore;