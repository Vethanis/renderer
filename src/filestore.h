#pragma once 

#include "store.h"
#include "loadfile.h"

struct NameStore{
    Store<const char*, 1024> m_store;
    unsigned add(const char* name){
        assert(m_store.full() == false);
        unsigned key = hash(name);
        m_store.insert(key, name);
        return key;
    }
    const char* get(unsigned name){
        const char** res = m_store.get(name);
        return res ? *res : nullptr;
    }
};

extern NameStore g_nameStore;