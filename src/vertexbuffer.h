#pragma once

#include "glm/glm.hpp"
#include "array.h"
#include "ints.h"

struct Vertex 
{
    glm::vec4 position;
    glm::vec4 normal;
    glm::vec4 color;

    void setPosition(const glm::vec3 val) { position.x = val.x; position.y = val.y; position.z = val.z; }
    void setNormal(const glm::vec3 val) { normal.x = val.x; normal.y = val.y; normal.z = val.z; }
    void setColor(const glm::vec3 val) { color.x = val.x; color.y = val.y; color.z = val.z; }
    void setMaterial(const glm::vec3 val){ position.w = val.x; normal.w = val.y; color.w = val.z; }
};

typedef Vector<Vertex> VertexBuffer;
typedef Vector<u32> IndexBuffer;

struct Geometry
{
    VertexBuffer vertices;
    IndexBuffer indices;
};