#pragma once

#include "asserts.h"
#include <cstdio>
#include "hash.h"
#include "ints.h"

template<typename T, s32 _capacity>
class Array
{
    T _data[_capacity];
    s32 _tail;
public:
    Array() : _tail(0){
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
    T& back(){ 
        Assert(_tail > 0);
        return _data[_tail - 1]; 
    }
    const T& back()const{ 
        Assert(_tail > 0);
        return _data[_tail - 1];
    }
    T& grow(){
        Assert(_tail < _capacity);
        ++_tail;
        return _data[_tail - 1];
    }
    T pop(){
        Assert(_tail > 0);
        --_tail;
        return _data[_tail];
    }
    T& operator[](s32 idx){
        Assert(idx >= 0 && idx < _tail);
        return _data[idx];
    }
    const T& operator[](s32 idx)const{
        Assert(idx >= 0 && idx < _tail);
        return _data[idx];
    }
    bool full()const{
        return _tail >= _capacity;
    }
    s32 count()const{
        return _tail;
    }
    s32 capacity()const{
        return _capacity;
    }
    s32 bytes()const{
        return sizeof(T) * _tail;
    }
    u32 hash()const{
        return fnv(_data, bytes());
    }
    void resize(s32 count){
        Assert(count <= _capacity);
        _tail = count;
    }
    void clear(){ _tail = 0; }
    s32 find(const T& t){
        for(s32 i = 0; i < _tail; ++i){
            if(_data[i] == t)
                return i;
        }
        return -1;
    }
    void uniquePush(const T& t){
        if(find(t) == -1){
            grow() = t;
        }
    }
    void remove(s32 idx){
        --_tail;
        _data[idx] = _data[_tail];
    }
    void findRemove(const T& t){
        s32 idx = find(t);
        if(idx != -1){
            remove(idx);
        }
    }
    void sort(s32 a, s32 b){
        if(a - b < 2)
            return;

        s32 i, j;
        {
            T& pivot = _data[(a + b) >> 1];
            for(i = a, j = b - 1; ; ++i, --j){
                while(_data[i] < pivot) ++i;
                while(_data[j] > pivot) --j;
    
                if(i >= j) break;
    
                T temp = _data[i];
                _data[i] = _data[j];
                _data[j] = temp;
            }
        }

        sort(a, i);
        sort(i, b);
    }
    void sort(){
        sort(0, _tail);
    }
    bool operator==(const Array& other)const{
        return hash() == other.hash();
    }
    void serialize(FILE* pFile){
        fwrite(&_tail, sizeof(u32), 1, pFile);
        fwrite(_data, sizeof(T), _tail, pFile);
    }
};

template<typename T>
class Vector
{
    T* _data;
    s32 _tail;
    s32 _capacity;
public:
    s32 capacity()const{ return _capacity; }
    s32 count()const{ return _tail; }
    bool full()const{ return _tail >= _capacity; }
    s32 bytes()const{ return sizeof(T) * _tail; }
    s32 hash()const{ return fnv(_data, bytes()); }

    T* begin(){ return _data; }
    const T* begin()const{ return _data; }
    T* end(){ return _data + _tail; }
    const T* end()const{ return _data + _tail; }
    T& back(){ 
        Assert(_tail > 0);
        return _data[_tail - 1]; 
    }
    const T& back()const{ 
        Assert(_tail > 0);
        return _data[_tail - 1];
    }
    T& operator[](s32 idx){
        Assert(idx >= 0 && idx < _tail);
        return _data[idx];
    }
    const T& operator[](s32 idx)const{
        Assert(idx >= 0 && idx < _tail);
        return _data[idx];
    }

    void resize(const s32 new_cap){
        if(!new_cap){
            delete[] _data;
            _data = nullptr;
        }
        else if(new_cap > _capacity){
            T* new_data = new T[new_cap];
            
            if(_data){
                for(s32 i = 0; i < _tail; ++i){
                    new_data[i] = _data[i];
                }
                delete[] _data;
            }
            _data = new_data;
        }
        _capacity = new_cap;
        _tail = _tail < _capacity ? _tail : _capacity;
    }

    T& append(){
        Assert(_tail < _capacity);
        ++_tail;
        return _data[_tail - 1];
    }
    T& grow(){
        if(_tail >= _capacity){
            resize(_tail ? _tail << 1 : 16);
        }
        ++_tail;
        return _data[_tail - 1];
    }
    T pop(){
        Assert(_tail > 0);
        --_tail;
        return _data[_tail];
    }
    void clear(){ _tail = 0; }
    void remove(s32 idx){
        Assert(idx <= _tail);
        --tail;
        _data[idx] = _data[tail];
    }
    s32 find(const T& t){
        for(s32 i = 0; i < _tail; ++i){
            if(_data[i] == t)
                return i;
        }
        return -1;
    }
    void uniquePush(const T& t){
        if(find(t) == -1){
            grow() = t;
        }
    }
    void findRemove(const T& t){
        s32 idx = find(t);
        if(idx != -1){
            remove(idx);
        }
    }
    void sort(s32 a, s32 b){
        if(a - b < 2)
            return;

        s32 i, j;
        {
            T& pivot = _data[(a + b) >> 1];
            for(i = a, j = b - 1; ; ++i, --j){
                while(_data[i] < pivot) ++i;
                while(_data[j] > pivot) --j;

                if(i >= j) break;

                T temp = _data[i];
                _data[i] = _data[j];
                _data[j] = temp;
            }
        }

        sort(a, i);
        sort(i, b);
    }
    void sort(){
        sort(0, _tail);
    }
    Vector() : _data(nullptr), _tail(0), _capacity(0){
    }
    Vector(s32 cap){
        if(cap > 0){
            _data = new T[cap];
        }
        else{
            _data = nullptr;
        }
        _capacity = cap;
        _tail = 0;
    }
    Vector(const Vector& other)
    {
        _data = nullptr;
        _tail = 0;
        copy(other);
    }
    Vector(Vector&& other) noexcept
    {
        _data = other._data;
        _tail = other._tail;
        _capacity = other._capacity;
        other._data = nullptr;
        other._tail = 0;
        other._capacity = 0;
    }
    ~Vector(){
        if(_data){
            delete[] _data;
            _data = nullptr;
        }
    }
    void copy(const Vector& other){
        if(_data){
            delete[] _data;
            _data = nullptr;
        }
        _tail = other._tail;
        _capacity = _tail;
        if(_tail){
            _data = new T[_capacity];
            for(s32 i = 0; i < _tail; ++i)
            {
                _data[i] = other._data[i];
            }
        }
    }
    Vector& operator=(const Vector& other){
        copy(other);
        return *this;
    }
    Vector& operator=(Vector&& other) noexcept
    {
        resize(0);
        _data = other._data;
        _tail = other._tail;
        _capacity = other._capacity;
        other._data = nullptr;
        other._tail = 0;
        other._capacity = 0;
    }
    bool operator==(const Vector& other)const{
        return hash() == other.hash();
    }
    void serialize(FILE* pFile){
        fwrite(&_tail, sizeof(s32), 1, pFile);
        if(_tail){
            fwrite(_data, sizeof(T), _tail, pFile);
        }
    }
    // does NOT work on nested vectors
    void serialize_composite(FILE* pFile){
        fwrite(&_tail, sizeof(s32), 1, pFile);
        for(s32 i = 0; i < _tail; ++i){
            _data[i].serialize(pFile);
        }
    }
    void load(FILE* pFile){
        if(_data){
            delete[] _data;
            _data = nullptr;
        }
        s32 count = 0;
        fread(&count, sizeof(s32), 1, pFile);
        _tail = count;
        _capacity = count;
        if(count){
            _data = new T[count];
            fread(_data, sizeof(T), count, pFile);
        }
    }
    // does NOT work on nested vectors
    void load_composite(FILE* pFile){
        if(_data){
            delete[] _data;
            _data = nullptr;
        }
        s32 count = 0;
        fread(&count, sizeof(s32), 1, pFile);
        _tail = count;
        _capacity = count;
        if(count){
            _data = new T[count];
            for(s32 i = 0; i < _tail; ++i){
                _data[i].load(pFile);
            }
        }
    }
    void reserve(s32 new_cap)
    {
        if(_capacity < new_cap)
        {
            T* new_data = new T[new_cap];
            for(s32 i = 0; i < _tail; ++i)
                new_data[i] = _data[i];

            delete[] _data;
            _data = new_data;
            _capacity = new_cap;
        }
    }
};