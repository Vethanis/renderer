#pragma once

#include "glm/glm.hpp"
#include <vector>
#include "store.h"

struct Vertex{
    glm::vec4 positionu;
    glm::vec4 normalv;
    glm::vec4 tangentm;
    glm::vec4 bitangent;
};

typedef std::vector<Vertex> VertexBuffer;