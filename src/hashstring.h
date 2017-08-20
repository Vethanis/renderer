#pragma once

struct HashString{
    unsigned m_hash;
    const char* str() const;
    bool valid()const{ return m_hash != 0; }
    HashString(){m_hash = 0;}
    HashString(unsigned hash){m_hash = hash;}
    HashString(const char* str);
    HashString operator = (HashString o) { m_hash = o.m_hash; return *this; }
    HashString operator = (unsigned hash) { m_hash = hash; return *this; }
    operator unsigned () const { return m_hash; }
    operator const char* () const { return str(); }

    template<typename T>
    operator T* () const;
};