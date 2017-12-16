#pragma once

#include "glm/glm.hpp"
#include "array.h"
#include "ints.h"

struct Vertex 
{
    glm::vec3 position;
    u32 normal;
    u32 color;
    u32 material;

    static u32 packUniVec3(glm::vec3 val)
    {
        u32 a = 0;
        val *= 255.0f;
        a = 0xff & u32(val.x);
        a <<= 8;
        a |= 0xff & u32(val.y);
        a <<= 8;
        a |= 0xff & u32(val.z);
        a <<= 8;
        // a |= 0xff & u32(val.w);

        return a;
    }
    static u32 packBiVec3(const glm::vec3 val)
    {
        return packUniVec3((val + 1.0f) * 0.5f);
    }
    static glm::vec3 unpackVec3Uni(u32 a)
    {
        glm::vec3 val;
        val.z = float(0xff & (a >> 8));
        val.y = float(0xff & (a >> 16));
        val.x = float(0xff & (a >> 24));
        return val / 255.0f;
    }
    static glm::vec3 unpackVec3Bi(u32 a)
    {
        glm::vec3 val = unpackVec3Uni(a);
        return val * 2.0f - 1.0f;
    }
    void setPosition(const glm::vec3 val) { position = val; }
    void setNormal(const glm::vec3 val) { normal = packBiVec3(val); }
    void setColor(const glm::vec3 val) { color = packUniVec3(val); }
    void setMaterial(const glm::vec3 val){ material = packUniVec3(val); }
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getNormal() const { return unpackVec3Bi(normal); }
    glm::vec3 getColor() const { return unpackVec3Uni(color); }
    glm::vec3 getMaterial() const { return unpackVec3Uni(material); }
};

typedef Vector<Vertex> VertexBuffer;
typedef Vector<u32> IndexBuffer;

struct Geometry
{
    VertexBuffer vertices;
    IndexBuffer indices;
};