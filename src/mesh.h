#pragma once

#include "common.h"
#include "store.h"
#include "assetloader.h"
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

void load_model(mesh_interchange::Model* model, unsigned name);

extern AssetStore<true, mesh_interchange::Model, 256> g_ModelStore;

struct MeshStore {
    Store<Mesh, 128> m_store;
    
    Mesh* get(HashString name){
        Mesh* m = m_store.get(name.m_hash);
        if(m){ return m; }

        if(g_ModelStore.m_mutex.try_lock())
        {
            mesh_interchange::Model* model = g_ModelStore[name.m_hash];

            if(model){
                if(m_store.full()){
                    m = m_store.reuse_near(name.m_hash);
                    m->upload(*model);
                }
                else{
                    m_store.insert(name.m_hash, {});
                    m = m_store[name.m_hash];
                    m->init();
                    m->upload(*model);
                }
            }
            g_ModelStore.m_mutex.unlock();
        }

        return m;
    }
    Mesh* operator[](HashString name){ return get(name); }
};

extern MeshStore g_MeshStore;