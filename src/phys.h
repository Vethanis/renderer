#pragma once

#include "glm/glm.hpp"

struct AABB{
    glm::vec3 min, max;
};

struct phys_object{
    glm::vec3 pos, vel;
    float mass;
}

struct PhysicsManager{
    void step(float dt){
    }
}
