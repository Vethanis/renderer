#pragma once 

#include "glm/glm.hpp"
#include "array.h"

struct light{
    glm::vec4 position, color;
};

typedef Array<light, 8> LightSet;
