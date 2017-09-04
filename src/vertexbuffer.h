#pragma once

#include "glm/glm.hpp"
#include "array.h"
#include "ints.h"

struct Vertex {
    glm::vec4 position;     // w = uv.x
    glm::vec4 normal;       // w = uv.y
    glm::vec3 tangent;
    unsigned matid;
};

typedef Vector<Vertex> VertexBuffer;
typedef Vector<u32> IndexBuffer;