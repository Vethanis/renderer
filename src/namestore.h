#pragma once 

#define _CRT_SECURE_NO_WARNINGS

#include "store.h"
#include "ints.h"
#include <cstring>

struct NameStore{
    Store<const char*, 1024> m_store;
    const char* get(unsigned name){
        const char** res = m_store.get(name);
        return res ? *res : nullptr;
    }
    const char* operator[](unsigned name){ return get(name); }
    void insert(unsigned name, const char* val){
        if(m_store.get(name)){
            return;
        }
        
        const u32 len = (u32)strlen(val);
        char* value = new char[len + 1];
        strcpy(value, val);
        m_store.insert(name, value);
    }
};

extern NameStore g_NameStore;
