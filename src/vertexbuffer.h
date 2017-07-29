#pragma once

#include "glm/glm.hpp"
#include <vector>
#include "store.h"

inline unsigned short toHalf(float f){
    unsigned* p = (unsigned*)&f;
    unsigned short v = (*p >> 31) << 5;
    unsigned short t = (*p >> 23) & 0xff;
    t = (t - 0x70) & ((unsigned)((int)(0x70 - t) >> 4) >> 27);
    v = (v | t) << 10;
    v |= (*p >> 13) & 0x3ff;
    return v;
}

struct half{
    unsigned short value;
    void set(float v){
        value = toHalf(v);
    }
    half(){};
    half(float v){
        set(v);
    }
};

struct half2 {
    half x, y;
    void set(const glm::vec2 v){
        x.set(v.x);
        y.set(v.y);
    }
    half2(){};
    half2(glm::vec2 v){
        set(v);
    }
    half2(float u, float v){
        x.set(u);
        y.set(v);
    }
};

struct half4 {
    half x, y, z, w;
    void set(const glm::vec4& v){
        x.set(v.x);
        y.set(v.y);
        z.set(v.z);
        w.set(v.w);
    }
    half4(){};
    half4(const glm::vec4& v){
        set(v);
    }
    half4(float a, float b, float c, float d){
        x.set(a);
        y.set(b);
        z.set(c);
        w.set(d);
    }
};

struct Vertex{
    half4 positionu;
    half4 normalv;
    half4 tangentm;
    half4 bitangent;
};

typedef std::vector<Vertex> VertexBuffer;