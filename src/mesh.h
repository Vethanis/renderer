#pragma once

#include "common.h"
#include "store.h"
#include "vertexbuffer.h"
#include "mesh_interchange.h"
#include "circular_queue.h"
#include <thread>
#include <mutex>
#include <chrono>

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

struct ModelStore {
    Store<mesh_interchange::Model, 256> m_store;
    CircularQueue<unsigned, 256> m_queue;
    std::thread m_thread;
    std::mutex m_mutex;
    bool m_shouldRun;

    void processQueue(){
        while(m_shouldRun){

            if(!m_queue.empty() && m_mutex.try_lock())
            {
                while(m_shouldRun && !m_queue.empty())
                {
                    unsigned name = m_queue.pop();
                    mesh_interchange::Model* m = nullptr;
                
                    if(m_store.full()){
                        m = m_store.reuse_near(name);
                    }
                    else{
                        m_store.insert(name, {});
                        m = m_store[name];
                    }
    
                    load_model(*m, name);
                }
                m_mutex.unlock();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    
    ModelStore(){
        m_shouldRun = true;
        m_thread = std::thread(&ModelStore::processQueue, this);
    }
    ~ModelStore(){
        m_shouldRun = false;
        m_thread.join();
    }
    void load_model(mesh_interchange::Model& model, unsigned name);
    mesh_interchange::Model* get(unsigned name){
        auto* m = m_store[name];
        if(m){ return m;}

        if(m_queue.full() == false){
            m_queue.set_push(name);
        }
        return nullptr;
    }
    mesh_interchange::Model* operator[](unsigned name){ return get(name); }
};

extern ModelStore g_ModelStore;

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