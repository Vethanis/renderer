#pragma once 

#include <cstring>
#include <cassert>
#include "hash.h"
#include <cstdio>

template<typename T, const unsigned cap>
struct Store{
    static constexpr unsigned invalid_val = 0xffffff;
    static constexpr unsigned MSB = 1 << 31;
    
    T data[cap];
    unsigned names[cap];
    unsigned count;
    unsigned capacity;

    unsigned getKey(int idx){ return names[idx]; }
    T& getValue(int idx){ return data[idx]; }
    unsigned getCapacity(){ return capacity; }
    bool validSlot(int idx){ return names[idx] != 0 && names[idx] != MSB; }
    bool is_deleted(unsigned key){
        return key == MSB;
    }
    unsigned mask(unsigned key){
        return key & (cap - 1);
    }
    unsigned probe_distance(unsigned pos, unsigned key){
        return mask(pos + cap - mask(key));
    }
    unsigned index_of(unsigned key){
        unsigned pos = mask(key);
        unsigned dist = 0;
        while(true){
            if(names[pos] == 0){
                return invalid_val;
            }
            else if(dist > probe_distance(pos, names[pos])){
                return invalid_val;
            }
            else if(names[pos] == key){
                return pos;
            }
            pos = mask(pos + 1);
            ++dist;
        }
        return invalid_val;
    }
    unsigned first_evictable(unsigned key){
        unsigned dist = 0;
        while(true){
            unsigned pos = mask(key + dist);
            unsigned existing_dist = probe_distance(pos, names[pos]);
            if(existing_dist < dist){
                return pos;
            }
            ++dist;
        }
        return invalid_val;
    }
    Store(){
        memset(names, 0, sizeof(unsigned) * cap);
        count = 0;
        capacity = cap;
    }

    struct iterator{
        Store* m_pStore;
        unsigned m_idx;

        void iterate(){
            for(; m_idx < m_pStore->capacity; ++m_idx){
                unsigned key = m_pStore->names[m_idx];
                if(key && key != MSB)
                    break;
            }
        }
        iterator(Store* pStore, unsigned idx = 0) : m_pStore(pStore), m_idx(idx){
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

    void insert(unsigned key, const T& _val){
        assert(count < cap);
        unsigned pos = mask(key);
        unsigned dist = 0;
        T val = _val;
        while(true){
            if(names[pos] == 0){
                names[pos] = key;
                data[pos] = val;
                count++;
                return;
            }

            unsigned existing_dist = probe_distance(pos, names[pos]);
            if(existing_dist < dist){
                if(is_deleted(names[pos])){
                    names[pos] = key;
                    data[pos] = val;
                    count++;
                    return;
                }

                {
                    const unsigned tname = key;
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
    void insert(const char* name, const T& val){
        insert(fnv(name), val);
    }
    T* get(unsigned key){
        unsigned loc = index_of(key);
        if(loc == invalid_val){
            return nullptr;
        }
        return data + loc;
    }
    T* get(const char* name){
        return get(fnv(name));
    }
    T* operator[](unsigned key){
        return get(key);
    }
    T* operator[](const char* name){
        return get(fnv(name));
    }
    T* remove(unsigned key){
        unsigned loc = index_of(key);
        if(loc != invalid_val){
            names[loc] = MSB;
            count--;
            return data + loc;
        }
        return nullptr;
    }
    T* remove(const char* name){
        return remove(fnv(name));
    }
    bool exists(unsigned key){
        unsigned loc = index_of(key);
        return loc != invalid_key;
    }
    bool exists(const char* name){
        return exists(fnv(name));
    }
    unsigned empty_slots(){
        return cap - count;
    }
    bool full(){ return cap == count; }
    T* remove_near(unsigned name){
        assert(full());
        unsigned loc = first_evictable(name);
        names[loc] = MSB;
        count--;
        return data + loc;
    }
    T* remove_near(const char* name){
        return remove_near(fnv(name));
    }
    T* reuse_near(unsigned name){
        assert(full());
        unsigned loc = first_evictable(name);
        names[loc] = name;
        return data + loc;
    }
    T* reuse_near(const char* name){
        return reuse_near(fnv(name));
    }
    unsigned grow(){
        assert(full() == false);
        static unsigned key = 1;
        while(get(key)){
            key = ((key + 1) & 0x7fffffff);
            key = key ? key : 1;
        }
        insert(key, {});
        return key;
    }
    void clear()
    {
        for(unsigned i = 0; i < count; ++i)
        {
            names[i] = 0;
        }
        for(unsigned i = 0; i < count; ++i)
        {
            data[i] = T();
        }
    }
};

inline void store_test(){
    constexpr unsigned c = 2048;
    Store<int, c> store;
    for(int i = 1; i <= c; i++){
        store.insert(i * 64, i * 64);
    }
    for(int i = 1; i <= c; i++){
        int* pi = store[i * 64];
        assert(pi);
        if(!(*pi == i * 64))__debugbreak();
    }
    for(int i = 1; i <= c; i++){
        int* pi = store.remove(i * 64);
        assert(pi);
        assert(*pi == i * 64);
    }
    puts("Done with store_test");
}