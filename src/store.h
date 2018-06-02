#pragma once 

#include "ints.h"
#include <cassert>

template<typename T, const u32 cap>
class Store
{
    static constexpr u32 invalid_val = 0xffffff;
    
    T data[cap];
    u32 names[cap];
    u32 count;
    u32 capacity;

    u32 mask(u32 key){
        return key & (cap - 1);
    }
    u32 probe_distance(u32 pos, u32 key){
        return mask(pos + cap - mask(key));
    }
    u32 index_of(u32 key){
        u32 pos = mask(key);
        u32 dist = 0;
        while(true){
            if(names[pos] == key){
                return pos;
            }
            else if(names[pos] == 0 || dist > probe_distance(pos, names[pos])){
                return invalid_val;
            }
            pos = mask(pos + 1);
            ++dist;
        }
        return invalid_val;
    }
public:
    Store(){
        count = 0;
        capacity = cap;
        for(u32 i = 0; i < capacity; ++i)
        {
            names[i] = 0;
        }
    }

    class iterator
    {
        Store* m_pStore;
        u32 m_idx;
        void iterate(){
            for(; m_idx < m_pStore->capacity; ++m_idx){
                u32 key = m_pStore->names[m_idx];
                if(key)
                    break;
            }
        }
    public:
        iterator(Store* pStore, u32 idx = 0) : m_pStore(pStore), m_idx(idx){
            iterate();
        }
        bool operator != (const iterator& o)const{
            return m_idx != o.m_idx;
        }
        T& operator*(){
            return m_pStore->data[m_idx];
        }
        iterator& operator++(){
            ++m_idx;
            iterate();
            return *this;
        }
    };

    iterator begin(){ return iterator(this); }
    iterator end(){ return iterator(this, capacity); }

    void insert(u32 key, const T& _val){
        assert(count < cap);
        u32 pos = mask(key);
        u32 dist = 0;
        T val = _val;
        while(true){
            if(names[pos] == 0){
                names[pos] = key;
                data[pos] = val;
                count++;
                return;
            }

            u32 existing_dist = probe_distance(pos, names[pos]);
            if(existing_dist < dist){
                {
                    const u32 tname = key;
                    key = names[pos];
                    names[pos] = tname;
                }
                {
                    const T tval = val;
                    val = data[pos];
                    data[pos] = tval;
                }

                dist = existing_dist;
            }

            pos = mask(pos + 1);
            ++dist;
        }
    }
    T* get(u32 key){
        u32 loc = index_of(key);
        if(loc == invalid_val){
            return nullptr;
        }
        return data + loc;
    }
    T* operator[](u32 key){
        return get(key);
    }
    void remove(u32 key){
        u32 loc = index_of(key);
        if(loc != invalid_val)
        {
            count--;
            names[loc] = 0;
            data[loc] = T();

            u32 nextLoc = mask(loc + 1);
            u32 trueLoc = mask(names[nextLoc]);
            while(names[nextLoc] && trueLoc != nextLoc)
            {
                names[loc] = names[nextLoc];
                names[nextLoc] = 0;
                data[loc] = data[nextLoc];
                data[nextLoc] = T();

                loc = nextLoc;
                nextLoc = mask(loc + 1);
                trueLoc = mask(names[nextLoc]);
            }
        }
    }
    bool full(){ return cap == count; }
    void clear()
    {
        for(u32 i = 0; i < count; ++i)
        {
            names[i] = 0;
        }
        for(u32 i = 0; i < count; ++i)
        {
            data[i] = T();
        }
    }
};