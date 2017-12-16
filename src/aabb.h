#pragma once

#include <glm/glm.hpp>

struct AABB
{
    glm::vec3 lo;
    glm::vec3 hi;

    glm::vec3 span() const { return hi - lo; }
    glm::vec3 center() const { return (hi + lo) * 0.5f; }
    float cornerRadius() const { return glm::distance(center(), hi); }
    float sideRadius() const 
    {
        const glm::vec3 c = hi - center();
        return glm::max(c.x, glm::max(c.y, c.z));
    }
    bool contains(glm::vec3 point) const
    {
        if(point.x > hi.x || point.x < lo.x)
            return false;
        if(point.y > hi.y || point.y < lo.y)
            return false;
        if(point.z > hi.z || point.z < lo.z)
            return false;
        return true;
    }
    void translate(glm::vec3 trans)
    {
        lo += trans;
        hi += trans;
    }
    void scale(glm::vec3 sc)
    {
        lo *= sc;
        hi *= sc;
    }
    bool validate()
    {
        bool valid = true;
        if(hi.x < lo.x)
        {
            float t = hi.x;
            hi.x = lo.x;
            lo.x = t;
            valid = false;
        }
        if(hi.y < lo.y)
        {
            float t = hi.y;
            hi.y = lo.y;
            lo.y = t;
            valid = false;
        }
        if(hi.z < lo.z)
        {
            float t = hi.z;
            hi.z = lo.z;
            lo.z = t;
            valid = false;
        }
        return valid;
    }
};