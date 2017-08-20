#pragma once 

#include "store.h"

struct NameStore{
    Store<const char*, 1024> m_store;
    
    const char* get(unsigned name){
        const char** res = m_store.get(name);
        return res ? *res : nullptr;
    }
    const char* operator[](unsigned name){ return get(name); }
    void insert(unsigned name, const char* val){
        m_store.insert(name, val);
    }
};

extern NameStore g_NameStore;
