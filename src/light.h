#pragma once 

#include "glm/glm.hpp"

struct light{
    glm::vec4 position, color;
};

constexpr int num_lights = 32;
struct LightSet{
    light lights[num_lights];
    int tail = 0;
    light& operator[](unsigned i){ return lights[i]; }
};
