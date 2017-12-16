#pragma once

#include "glm/glm.hpp"
#include "array.h"
#include "ints.h"

struct Vertex 
{
    float x, y, z;
    s8 nx, ny, nz, _pad1;
    u8 red, green, blue, _pad2;
    u8 roughness, metalness, ao, _pad3;

    void setPosition(glm::vec3 val) { x = val.x; y = val.y; z = val.z; }
    void setNormal(glm::vec3 val) { val *= 127.0f; nx = (u8)val.x; ny = (u8)val.y; nz = (u8)val.z; }
    void setColor(glm::vec3 val) { val *= 255.0f; red = (u8)val.x; green = (u8)val.y; blue = (u8)val.z; }
    void setRoughness(float val) { roughness = (u8)(val * 255.0f);}
    void setMetalness(float val) { metalness = (u8)(val* 255.0f);}
    void setAmbientOcclusion(float val) { ao = (u8)(val * 255.0f);}
    glm::vec3 getPosition() const { return glm::vec3(x, y, z); }
    glm::vec3 getNormal() const { return glm::vec3(nx / 127.0f, ny / 127.0f, nz / 127.0f); }
    glm::vec3 getColor() const { return glm::vec3(red / 255.0f, green / 255.0f, blue / 255.0f); }
    float getRoughness() const { return roughness / 255.0f; }
    float getMetalness() const { return metalness / 255.0f; }
    float getAmbientOcclusion() const { return ao / 255.0f; }
};

typedef Vector<Vertex> VertexBuffer;
typedef Vector<u32> IndexBuffer;

struct Geometry
{
    VertexBuffer vertices;
    IndexBuffer indices;
};