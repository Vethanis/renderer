#pragma once 

#include "ints.h"
#include <cstring>
#include "asserts.h"
#include "hash.h"

template<typename T, const u32 cap>
class Store
{
    static constexpr u32 invalid_val    = 0xffffffff;
    static constexpr u32 Tombstone      = 0xffffffff;
    
    T data[cap];
    u32 names[cap];
    u32 count;
    u32 capacity;

public:
    u32 getKey(s32 idx){ return names[idx]; }
    T& getValue(s32 idx){ return data[idx]; }
    u32 getCapacity(){ return capacity; }
    bool validSlot(s32 idx){ return names[idx] != 0 && names[idx] != Tombstone; }
    bool is_deleted(u32 key){
        return key == Tombstone;
    }
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
    u32 first_evictable(u32 key){
        u32 dist = 0;
        while(true){
            u32 pos = mask(key + dist);
            u32 existing_dist = probe_distance(pos, names[pos]);
            if(existing_dist < dist){
                return pos;
            }
            ++dist;
        }
        return invalid_val;
    }
    Store(){
        memset(names, 0, sizeof(u32) * cap);
        count = 0;
        capacity = cap;
    }

    struct iterator{
        Store* m_pStore;
        u32 m_idx;

        void iterate(){
            for(; m_idx < m_pStore->capacity; ++m_idx){
                u32 key = m_pStore->names[m_idx];
                if(key && key != Tombstone)
                    break;
            }
        }
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

    void insert(u32 key, T&& val) noexcept
    {
        Assert(count < cap);
        u32 pos = mask(key);
        u32 dist = 0;
        while(true)
        {
            if(names[pos] == 0)
            {
                names[pos] = key;
                data[pos] = val;
                count++;
                return;
            }

            u32 existing_dist = probe_distance(pos, names[pos]);
            if(existing_dist < dist)
            {
                if(is_deleted(names[pos]))
                {
                    names[pos] = key;
                    data[pos] = val;
                    count++;
                    return;
                }

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
    void insert(const char** name, const T& val){
        insert(fnv(name), val);
    }
    T* get(u32 key){
        u32 loc = index_of(key);
        if(loc == invalid_val){
            return nullptr;
        }
        return data + loc;
    }
    T* get(const char** name){
        return get(fnv(name));
    }
    T* operator[](u32 key){
        return get(key);
    }
    T* operator[](const char** name){
        return get(fnv(name));
    }
    T* remove(u32 key){
        u32 loc = index_of(key);
        if(loc != invalid_val){
            names[loc] = Tombstone;
            count--;
            return data + loc;
        }
        return nullptr;
    }
    T* remove(const char** name){
        return remove(fnv(name));
    }
    bool exists(u32 key){
        u32 loc = index_of(key);
        return loc != invalid_key;
    }
    bool exists(const char** name){
        return exists(fnv(name));
    }
    u32 empty_slots(){
        return cap - count;
    }
    bool full(){ return cap == count; }
    T* remove_near(u32 name){
        Assert(full());
        u32 loc = first_evictable(name);
        names[loc] = Tombstone;
        count--;
        return data + loc;
    }
    T* remove_near(const char** name){
        return remove_near(fnv(name));
    }
    T* reuse_near(u32 name){
        Assert(full());
        u32 loc = first_evictable(name);
        names[loc] = name;
        return data + loc;
    }
    T* reuse_near(const char** name){
        return reuse_near(fnv(name));
    }
    u32 grow(){
        Assert(full() == false);
        static u32 key = 1;
        while(get(key)){
            key = ((key + 1) & 0x7fffffff);
            key = key ? key : 1;
        }
        insert(key, {});
        return key;
    }
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

inline void store_test(){
    constexpr u32 c = 2048;
    Store<s32, c> store;
    for(s32 i = 1; i <= c; i++){
        store.insert(i * 64, i * 64);
    }
    for(s32 i = 1; i <= c; i++){
        s32* pi = store[i * 64];
        Assert(pi);
        if(!(*pi == i * 64))__debugbreak();
    }
    for(s32 i = 1; i <= c; i++){
        s32* pi = store.remove(i * 64);
        Assert(pi);
        Assert(*pi == i * 64);
    }
    puts("Done with store_test");
}