#pragma once 

#include "store.h"

struct NameStore
{
    struct TextBlock
    {
        enum : int
        {
            Capacity = 256,
        };
        char m_text[Capacity] = {0};
    };

    Store<TextBlock, 256> m_store;

    const char* get(unsigned name)
    {
        TextBlock* res = m_store.get(name);
        return res ? res->m_text : nullptr;
    }
    const char* operator[](unsigned name){ return get(name); }
    void insert(unsigned name, const char* val)
    {
        if(m_store.get(name))
        {
            return;
        }
        
        TextBlock block;
        for(int i = 0; i < TextBlock::Capacity - 1 && val[i]; ++i)
        {
            block.m_text[i] = val[i];
        }

        m_store.insert(name, block);
    }
};

extern NameStore g_NameStore;
