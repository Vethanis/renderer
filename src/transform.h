#pragma once

#include "store.h"
#include "glm/glm.hpp"

typedef glm::mat4 Transform;

struct TransformStore{
    Store<Transform, 1024> m_store;

    Transform* get(unsigned key){
        return m_store[key];
    }
    Transform* operator[](unsigned key){
        return get(key);
    }
    unsigned grow(){
        return m_store.grow();
    }
    void release(unsigned id){
        m_store.remove(id);
    }
    bool full(){
        return m_store.full();
    }
};

extern TransformStore g_TransformStore;