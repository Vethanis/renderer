#pragma once

#include <cassert>
#include "hash.h"

template<typename T, unsigned _capacity>
struct Array{
    T data[_capacity];
    unsigned tail;
    Array() : tail(0){
    }
    T& grow(){
        assert(tail < _capacity);
        return data[tail++];
    }
    T* begin(){
        return data;
    }
    T* end(){
        return data + tail;
    }
    const T* begin()const{
        return data;
    }
    const T* end()const{
        return data + tail;
    }
    T& operator[](unsigned idx){
        return data[idx];
    }
    const T& operator[](unsigned idx)const{
        return data[idx];
    }
    bool full()const{
        return tail >= _capacity;
    }
    unsigned count()const{
        return tail;
    }
    unsigned capacity()const{
        return _capacity;
    }
    unsigned bytes()const{
        return sizeof(T) * tail;
    }
    unsigned hash()const{
        return fnv(data, bytes());
    }
    void resize(unsigned count){
        tail = count;
    }
};