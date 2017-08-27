#pragma once

#include <cassert>
#include "hash.h"

template<typename T, int _capacity>
struct Array{
    T _data[_capacity];
    int _tail;
    Array() : _tail(0){
    }
    T& grow(){
        assert(_tail < _capacity);
        return _data[_tail++];
    }
    T* begin(){
        return _data;
    }
    T* end(){
        return _data + _tail;
    }
    const T* begin()const{
        return _data;
    }
    const T* end()const{
        return _data + _tail;
    }
    T& operator[](int idx){
        return _data[idx];
    }
    const T& operator[](int idx)const{
        return _data[idx];
    }
    bool full()const{
        return _tail >= _capacity;
    }
    int count()const{
        return _tail;
    }
    int capacity()const{
        return _capacity;
    }
    int bytes()const{
        return sizeof(T) * _tail;
    }
    unsigned hash()const{
        return fnv(_data, bytes());
    }
    void resize(int count){
        _tail = count;
    }
    void clear(){ _tail = 0; }
    int find(const T& t){
        for(int i = 0; i < _tail; ++i){
            if(_data[i] == t)
                return i;
        }
        return -1;
    }
    void sort(int a, int b){
        if(a - b < 2)
            return;

        T& pivot = _data[(a + b) >> 1];

        int i, j;
        for(i = a, j = b - 1; ; ++i, --j){
            while(_data[i] < pivot) ++i;
            while(_data[j] > pivot) --j;

            if(i >= j) break;

            T temp = _data[i];
            _data[i] = _data[j];
            _data[j] = temp;
        }

        sort(a, i);
        sort(i, b);
    }
    void sort(){
        sort(0, _tail);
    }
    bool operator==(const Array& other)const{
        return begin() == other.begin();
    }
};

template<typename T>
struct Vector{
    T* _data;
    int _tail;
    int _capacity;

    int capacity()const{ return _capacity; }
    int count()const{ return _tail; }
    bool full()const{ return _tail >= _capacity; }
    int bytes()const{ return sizeof(T) * _tail; }
    int hash()const{ return fnv(_data, bytes()); }

    T* begin(){ return _data; }
    const T* begin()const{ return _data; }
    T* end(){ return _data + _tail; }
    const T* end()const{ return _data + _tail; }
    T& back(){ 
        assert(_tail > 0);
        return _data[_tail - 1]; 
    }
    const T& back()const{ 
        assert(_tail > 0);
        return _data[_tail - 1];
    }
    T& operator[](int idx){
        return _data[idx];
    }
    const T& operator[](int idx)const{
        return _data[idx];
    }

    void resize(const int new_cap){
        if(!new_cap){
            delete[] _data;
            _data = nullptr;
        }
        else if(new_cap > _capacity){
            T* new_data = new T[new_cap];
            
            if(_data){
                for(int i = 0; i < _tail; ++i){
                    new_data[i] = _data[i];
                }
                delete[] _data;
                _data = new_data;
            }
            else{
                _data = new_data;
            }
        }
        _capacity = new_cap;
        _tail = _tail < _capacity ? _tail : _capacity;
    }

    T& append(){
        assert(_tail < _capacity);
        return _data[_tail++];
    }
    T& grow(int step = 16){
        if(_tail >= _capacity){
            resize(_tail + step);
        }
        return _data[_tail++];
    }
    T& pop(){
        assert(_tail > 0);
        return _data[_tail--];
    }
    void remove(int idx){
        assert(idx <= _tail);
        _data[idx] = back();
        --_tail;
    }
    void clear(){ _tail = 0; }
    int find(const T& t){
        for(int i = 0; i < _tail; ++i){
            if(_data[i] == t)
                return i;
        }
        return -1;
    }
    void sort(int a, int b){
        if(a - b < 2)
            return;

        T& pivot = _data[(a + b) >> 1];

        int i, j;
        for(i = a, j = b - 1; ; ++i, --j){
            while(_data[i] < pivot) ++i;
            while(_data[j] > pivot) --j;

            if(i >= j) break;

            T temp = _data[i];
            _data[i] = _data[j];
            _data[j] = temp;
        }

        sort(a, i);
        sort(i, b);
    }
    void sort(){
        sort(0, _tail);
    }
    Vector() : _data(nullptr), _tail(0), _capacity(0){
    }
    Vector(int cap){
        if(cap > 0){
            _data = new T[cap];
        }
        else{
            _data = nullptr;
        }
        _capacity = cap;
        _tail = 0;
    }
    ~Vector(){
        delete[] _data;
    }
    bool operator==(const Vector& other)const{
        return begin() == other.begin();
    }
};