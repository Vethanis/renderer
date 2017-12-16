#pragma once

#include "ints.h"
#include "assert.h"
#include "hash.h"

template<typename T, const u32 t_capacity, typename C = u16>
class TwArray
{
private:
    T m_data[t_capacity];
    C m_twists[t_capacity];
    C m_tail;
    C m_lastTwist;
public:
    TwArray()
    {
        m_tail = 0;
        m_lastTwist = 0;
    }
    C count() const
    {
        return m_tail;
    }
    C bytes() const
    {
        return sizeof(T) * m_tail;
    }
    u32 hash() const
    {
        return fnv(m_data, bytes());
    }
    bool full() const
    {
        return m_tail == t_capacity;
    }
    bool empty() const
    {
        return m_tail == 0;
    }
    bool valid(C idx) const
    {
        return idx < m_tail && m_twists[idx] < m_tail;
    }
    const T* begin() const
    {
        return m_data;
    }
    T* begin()
    {
        return m_data;
    }
    const T* end() const
    {
        return m_data + m_tail;
    }
    T* end()
    {
        return m_data + m_tail;
    }
    const T& back() const
    {
        assert(count());
        return m_data[m_tail - 1];
    }
    T& back()
    {
        assert(count());
        return m_data[m_tail - 1];
    }
    const T& operator[](C idx) const
    {
        const C pos = m_twists[idx];
        return m_data[pos];
    }
    T& operator[](C idx)
    {
        const C pos = m_twists[idx];
        return m_data[pos];
    }
    void clear()
    {
        m_tail = 0;
    }
    C insert(const T& item)
    {
        assert(!full());
        const C pos = m_tail;
        ++m_tail;
        m_data[pos] = item;
        m_twists[pos] = pos;

        m_lastTwist = m_lastTwist > pos ? m_lastTwist : pos;

        return pos;
    }
    void remove(C idx)
    {
        assert(count());
        const C pos = m_twists[idx];
        const C tail = m_tail - 1;
        m_data[pos] = m_data[tail];
        m_twists[idx] = (C)-1;
        m_twists[tail] = pos;

        if(pos == m_lastTwist)
        {
            while(m_twists[m_lastTwist] == (C)-1 && m_lastTwist)
            {
                --m_lastTwist;
            }
        }
    }
    C numSlots() const { return m_lastTwist; }
    C getSlotOf(const T& item) const
    {
        const size_t offset = size_t(&item - m_data);
        assert(offset < (size_t)m_tail);

        for(C i = (C)offset; i < m_lastTwist; ++i)
        {
            if(m_twists[i] == (C)offset)
            {
                return i;
            }
        }
        return (C)-1;
    }
};