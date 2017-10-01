#pragma once

#include "glm/glm.hpp"
#include "array.h"
#include "ints.h"

struct Vertex {
    glm::vec4 position;     // w = uv.x
    glm::vec4 normal;       // w = uv.y
};

typedef Vector<Vertex> VertexBuffer;
typedef Vector<u32> IndexBuffer;