#pragma once

#include "glm/glm.hpp"
#include <vector>
#include "store.h"

struct Vertex{
    glm::vec3 position, normal;
    glm::vec2 uv;
};

typedef std::vector<Vertex> VertexBuffer;