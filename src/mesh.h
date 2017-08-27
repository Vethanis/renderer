#pragma once

#include "store.h"
#include "vertexbuffer.h"

struct Mesh {
    unsigned vao, vbo, num_vertices;
    void draw();
    void upload(const VertexBuffer& vb);
    void init();
    void deinit();
    bool operator==(const Mesh& other)const{
        return vao == other.vao;
    }
};

struct MeshStore{
    Store<Mesh, 32> m_store;
    Store<VertexBuffer, 256> m_vbs;
    
    void load_mesh(Mesh& mesh, unsigned name);
    Mesh* get(unsigned name){
        Mesh* m = m_store.get(name);
        if(m){ return m; }

        if(m_store.full()){
            m = m_store.reuse_near(name);
        }
        else{
            m_store.insert(name, {});
            m = m_store.get(name);
            m->init();
        }

        load_mesh(*m, name);
        return m;
    }
    Mesh* operator[](unsigned name){ return get(name); }
    Mesh* operator[](const char* name){ return get(fnv(name)); }
};

extern MeshStore g_MeshStore;