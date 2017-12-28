#pragma once

#include "ints.h"
#include "asserts.h"
#include "stdio.h"

struct HashString
{
    u32 m_hash;
    const char* str() const;
    bool valid()const{ return m_hash != 0; }
    HashString(){m_hash = 0;}
    HashString(u32 hash){m_hash = hash;}
    HashString(const char* str);
    HashString operator = (HashString o) { m_hash = o.m_hash; return *this; }
    HashString operator = (const char* str) { *this = HashString(str); return *this; }
    HashString operator = (u32 hash) { m_hash = hash; return *this; }
    operator u32 () const { return m_hash; }
    bool operator==(HashString other)const{ return m_hash == other.m_hash; }
};