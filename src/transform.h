#pragma once

#include "store.h"
#include "glm/glm.hpp"

typedef glm::mat4 Transform;

struct TransformStore{
    Store<Transform, 1024> m_store;
    unsigned tail;

    Transform* get(unsigned key){
        return m_store[key];
    }
    Transform* operator[](unsigned key){
        return get(key);
    }
    unsigned grow(){
        assert(m_store.full() == false);
        tail = (tail & 0x7fffffff) | 1;
        
        while(m_store.get(tail)){
            tail = ((tail + 1) & 0x7fffffff) | 1;
        }
        m_store.insert(tail, {});
        return tail++;
    }
    void release(unsigned id){
        m_store.remove(id);
    }
    bool full(){
        return m_store.full();
    }
};

extern TransformStore g_TransformStore;