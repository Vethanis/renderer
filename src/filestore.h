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
    const char* operator[](unsigned name){ return get(name); }
    NameStore(){
        add("albedoSampler0");
        add("albedoSampler1");
        add("albedoSampler2");
        add("albedoSampler3");
        add("normalSampler0");
        add("normalSampler1");
        add("normalSampler2");
        add("normalSampler3");
        add("MVP");
        add("eye");
        add("positionSampler");
        add("normalSampler");
        add("materialSampler");
    }
};

extern NameStore g_nameStore;