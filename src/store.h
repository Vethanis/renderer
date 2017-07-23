#pragma once 

#include <algorithm>
#include <cstring>
#include <cassert>
#include "hash.h"

template<typename T, const unsigned cap>
class Store{
    unsigned names[cap];
    T data[cap];
    unsigned count;

    unsigned mask(unsigned key){
        return key & (cap - 1);
    }
    unsigned probe_distance(unsigned key){
        unsigned dist = 0;
        unsigned pos = mask(key);
        while(names[pos] && names[pos] != key){
            pos = mask(pos + 1);
            ++dist;
        }
        return dist;
    }
    unsigned index_of(unsigned key){
        unsigned pos = mask(key);
        while(names[pos] && names[pos] != key){
            pos = mask(pos + 1);
        }
        return pos;
    }
public:
    Store(){
        memset(names, 0, sizeof(unsigned) * cap);
    }
    void insert(unsigned key, const T& _val){
        assert(count < cap);
        unsigned pos = mask(key);       
        if(names[pos] == 0){
            names[pos] = key;
            data[pos] = _val;
            count++;
            return;
        }
        T val = _val;
        unsigned dist = 0;
        for(;;){
            if(names[pos] == 0){
                names[pos] = key;
                data[pos] = val;
                count++;
                return;
            }

            unsigned existing_dist = probe_distance(pos);
            if(existing_dist < dist){
                std::swap(key, names[pos]);
                std::swap(val, data[pos]);
                dist = existing_dist;
            }

            pos = mask(pos + 1);
            ++dist;
        }
    }
    void insert(const char* name, const T& val){
        insert(hash(name), val);
    }
    T* get(unsigned key){
        unsigned loc = index_of(key);
        if(names[loc] == key){
            return data + loc;
        }
        return nullptr;
    }
    T* get(const char* name){
        return get(hash(name));
    }
    T* operator[](unsigned key){
        return get(key);
    }
    T* operator[](const char* name){
        return get(hash(name));
    }
    T* remove(unsigned key){
        unsigned loc = index_of(key);
        if(names[loc] == key){
            names[loc] = 0;
            count--;
            return data + loc;
        }
    }
    T* remove(const char* name){
        return remove(hash(name));
    }
    bool exists(unsigned key){
        unsigned loc = index_of(key);
        return names[loc] == key;
    }
    bool exists(const char* name){
        return exists(hash(name));
    }
    unsigned empty_slots(){
        return cap - count;
    }
    bool full(){ return cap == count; }
    T* remove_near(unsigned name){
        assert(full());
        unsigned loc = mask(name);
        names[loc] = 0;
        count--;
        return data + loc;
    }
    T* remove_near(const char* name){
        return remove_near(hash(name));
    }
};
