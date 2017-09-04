#pragma once

#include "common.h"
#include "store.h"
#include "vertexbuffer.h"
#include "mesh_interchange.h"

// just some vertices on the GPU, nothing more
struct Mesh {
    u32 vao, vbo, ebo, num_indices;
    void draw();
    void upload(const mesh_interchange::Model& vb);
    void init();
    void deinit();
    bool operator==(const Mesh& other)const{
        return vao == other.vao;
    }
};

struct MeshStore {
    Store<Mesh, 128> m_store;
    Store<mesh_interchange::Model, 256> m_vbs;
    
    void load_mesh(Mesh& mesh, HashString name);
    Mesh* get(HashString name){
        Mesh* m = m_store.get(name.m_hash);
        if(m){ return m; }

        if(m_store.full()){
            m = m_store.reuse_near(name.m_hash);
        }
        else{
            m_store.insert(name.m_hash, {});
            m = m_store.get(name.m_hash);
            m->init();
        }

        load_mesh(*m, name);
        return m;
    }
    Mesh* operator[](HashString name){ return get(name); }
};

extern MeshStore g_MeshStore;