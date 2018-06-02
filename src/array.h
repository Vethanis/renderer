#pragma once

#include <cassert>
#include "hash.h"
#include "ints.h"

template<typename T>
class Slice
{
    const T* m_begin;
    const T* m_end;
public:
    Slice() : m_begin(nullptr), m_end(nullptr) {};
    Slice(const T* begin, const T* end) : m_begin(begin), m_end(end) {};
    Slice(const T* begin, s32 count) : m_begin(begin), m_end(begin + count) {};
    Slice(const T* item) : m_begin(item), m_end(item + 1) {};
    Slice(const T& item) : m_begin(&item), m_end((&item) + 1) {};
    const T* begin() const { return m_begin; }
    const T* end() const { return m_end; }
    size_t count() const { return size_t(m_end - m_begin); }
    const T& operator[](s32 idx) const { return m_begin[idx]; }
};

template<typename T>
static void QuickSort(T* x, s32 len)
{
    if(len <= 8)
    {
        for(s32 i = 0; i < len; ++i)
        {
            s32 c = i;
            for(s32 j = i + 1; j < len; ++j)
            {
                if(x[j] < x[c])
                {
                    c = j;
                }
            }
            T tmp = x[c];
            x[c] = x[i];
            x[i] = tmp;
        }
        return;
    }

    s32 i, j;
    {
        const T& pivot = x[len / 2];
        for(i = 0, j = len - 1; ; ++i, --j)
        {
            while(x[i] < pivot) ++i;
            while(x[j] > pivot) --j;

            if(i >= j) 
                break;

            T temp = x[i];
            x[i] = x[j];
            x[j] = temp;
        }
    }

    QuickSort(x, i);
    QuickSort(x + i, len - i);
}

template<typename T, s32 t_capacity>
class Array
{
    T m_data[t_capacity];
    s32 m_tail;
public:
    Array() : m_tail(0) {}
    T* begin() { return m_data; }
    T* end() { return m_data + m_tail; }
    const T* begin() const { return m_data; }
    const T* end() const { return m_data + m_tail; }
    T& operator[](s32 idx) { return m_data[idx]; }
    const T& operator[](s32 idx) const { return m_data[idx]; }
    s32 count() const { return m_tail; }
    s32 capacity() const { return t_capacity; }
    bool full() const { return count() >= capacity(); }
    bool empty() const { return count() == 0; }
    size_t bytes() const { return sizeof(T) * size_t(count()); }
    u32 hash() const { return fnv(begin(), bytes()); }
    Slice<T> toSlice() const { return { begin(), end() }; }
    operator Slice<T>() const { return toSlice(); }
    void clear() { m_tail = 0; }
    void sort() { QuickSort(begin(), count()); }
    T& back()
    { 
        assert(count() > 0);
        return m_data[m_tail - 1]; 
    }
    const T& back() const
    { 
        assert(count() > 0);
        return m_data[m_tail - 1];
    }
    T& grow()
    {
        assert(count() < t_capacity);
        ++m_tail;
        return back();
    }
    T& pop()
    {
        assert(count() > 0);
        T& val = back();
        --m_tail;
        return val;
    }
    void popfast()
    {
        assert(count() > 0);
        --m_tail;
    }
    void resize(s32 count)
    {
        assert(count <= t_capacity);
        m_tail = count;
    }
    s32 find(const T& t)
    {
        for(s32 i = 0; i < m_tail; ++i)
        {
            if(m_data[i] == t)
                return i;
        }
        return -1;
    }
    void uniquePush(const T& t)
    {
        if(find(t) == -1)
        {
            grow() = t;
        }
    }
    void remove(s32 idx)
    {
        --m_tail;
        m_data[idx] = m_data[m_tail];
    }
    void findRemove(const T& t)
    {
        s32 idx = find(t);
        if(idx != -1)
        {
            remove(idx);
        }
    }
};

template<typename T>
class Vector
{
    T* m_data;
    s32 m_tail;
    s32 m_capacity;
public:
    Vector() : m_data(nullptr), m_tail(0), m_capacity(0) {}
    ~Vector() { delete[] m_data; }
    s32 capacity() const { return m_capacity; }
    s32 count() const { return m_tail; }
    bool full() const { return count() >= capacity(); }
    bool empty() const { return count() == 0; }
    size_t bytes() const { return sizeof(T) * size_t(count()); }
    T* begin() { return m_data; }
    const T* begin() const { return m_data; }
    T* end() { return m_data + m_tail; }
    const T* end() const { return m_data + m_tail; }
    T& operator[](s32 idx) { return m_data[idx]; }
    const T& operator[](s32 idx) const { return m_data[idx]; }
    u32 hash() const { return fnv(begin(), bytes()); }
    Slice<T> toSlice() const { return { begin(), end() }; }
    operator Slice<T>() const { return toSlice(); }
    void clear(){ m_tail = 0; }
    void sort() { QuickSort(begin(), count()); }
    T& back()
    { 
        assert(count() > 0);
        return m_data[m_tail - 1]; 
    }
    const T& back() const
    { 
        assert(count() > 0);
        return m_data[m_tail - 1];
    }
    void reserve(const s32 new_cap)
    {
        const s32 new_tail = new_cap < m_tail ? new_cap : m_tail;
        if(!new_cap)
        {
            delete[] m_data;
            m_data = nullptr;
        }
        else
        {
            T* new_data = new T[new_cap];
            for(s32 i = 0; i < new_tail; ++i)
            {
                new_data[i] = m_data[i];
            }
            delete[] m_data;
            m_data = new_data;
        }
        m_capacity = new_cap;
        m_tail = new_tail;
    }
    void resize(const s32 new_size)
    {
        if(new_size > m_capacity)
        {
            reserve(new_size);
        }
        m_tail = new_size;
    }
    T& append()
    {
        assert(m_tail < m_capacity);
        ++m_tail;
        return back();
    }
    T& grow()
    {
        if(m_tail >= m_capacity)
        {
            reserve(m_tail ? m_tail * 2 : 16);
        }
        ++m_tail;
        return back();
    }
    T& pop()
    {
        assert(count() > 0);
        T& item = back();
        --m_tail;
        return item;
    }
    void popfast()
    {
        assert(count() > 0);
        --m_tail;
    }
    void reset()
    {
        delete[] m_data;
        m_data = nullptr;
        m_capacity = 0;
        m_tail = 0;
    }
    void remove(s32 idx)
    {
        assert(idx <= m_tail);
        m_data[idx] = back();
        --m_tail;
    }
    s32 find(const T& t)
    {
        for(s32 i = 0; i < m_tail; ++i)
        {
            if(m_data[i] == t)
                return i;
        }
        return -1;
    }
    void uniquePush(const T& t)
    {
        if(find(t) == -1)
        {
            grow() = t;
        }
    }
    void findRemove(const T& t)
    {
        s32 idx = find(t);
        if(idx != -1)
        {
            remove(idx);
        }
    }
    Vector(const Vector& other)
    {
        m_data = nullptr;
        m_tail = other.count();
        m_capacity = other.capacity();
        if(m_capacity)
        {
            m_data = new T[m_capacity];
            for(s32 i = 0; i < m_tail; ++i)
            {
                m_data[i] = other[i];
            }
        }
    }
    Vector(Vector&& other)
    {
        m_tail = other.count();
        m_capacity = other.capacity();
        m_data = other.m_data;
        other.m_data = nullptr;
        other.m_tail = 0;
        other.m_capacity = 0;
    }
    void copy(const Vector& other)
    {
        delete[] m_data;
        m_data = nullptr;
        m_tail = other.count();
        m_capacity = other.capacity();
        if(m_capacity){
            m_data = new T[m_capacity];
            for(s32 i = 0; i < m_tail; ++i)
            {
                m_data[i] = other[i];
            }
        }
    }
    void assume(Vector& other)
    {
        reset();
        m_tail = other.count();
        m_capacity = other.capacity();
        m_data = other.m_data;
        other.m_data = nullptr;
        other.m_tail = 0;
        other.m_capacity = 0;
    }
    Vector& operator=(const Vector& other)
    {
        delete[] m_data;
        m_data = nullptr;
        m_tail = other.count();
        m_capacity = other.capacity();
        if(m_capacity)
        {
            m_data = new T[m_capacity];
            for(s32 i = 0; i < m_tail; ++i)
            {
                m_data[i] = other[i];
            }
        }
        return *this;
    }
    Vector& operator=(Vector&& other) noexcept 
    {
        reset();
        m_tail = other.count();
        m_capacity = other.capacity();
        m_data = other.m_data;
        other.m_data = nullptr;
        other.m_tail = 0;
        other.m_capacity = 0;
        return *this;
    }
};