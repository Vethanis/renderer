#pragma once

#include "ints.h"
#include "array.h"
#include "linmath.h"

enum SDFType : u8
{
    SDF_SPHERE = 0,
    SDF_BOX,
    SDF_COUNT
};

enum SDFBlend : u8
{
    SDF_UNION = 0,
    SDF_DIFF,
    SDF_INTER,
    SDF_S_UNION,
    SDF_S_DIFF,
    SDF_S_INTER,
    SDF_BLEND_COUNT
};

struct Material
{
    u8 red=0, green=0, blue=0;
    u8 roughness=0;
    u8 metalness=0;
    void setColor(vec3 c)
    {
        red = (u8)(c.x * 255.0f);
        green = (u8)(c.y * 255.0f);
        blue = (u8)(c.z * 255.0f);
    }
    void setRoughness(float x)
    {
        roughness = (u8)(x * 255.0f);
    }
    void setMetalness(float x)
    {
        metalness = (u8)(x * 255.0f);
    }
    vec3 getColor()const
    {
        return vec3(float(red), float(green), float(blue)) / 255.0f;
    }
    float getRoughness()const
    {
        return roughness / 255.0f;
    }
    float getMetalness()const
    {
        return metalness / 255.0f;
    }
};

struct SDF
{
    vec3 translation;
    vec3 scale = vec3(1.0f);
    vec3 rotation;
    float smoothness = 0.005f;
    Material material;
    SDFType type = SDF_SPHERE;
    SDFBlend blend_type = SDF_UNION;

    float distance(vec3 p)const;
    float blend(float a, float b)const;
    vec3 max()const;
    vec3 min()const;
};

typedef Vector<SDF> SDFList;
typedef Vector<u16> SDFIndices;

inline void findBasis(vec3 N, vec3& T, vec3& B)
{
    if(glm::abs(N.x) > 0.001f)
        T = glm::cross(vec3(0.0f, 1.0f, 0.0f), N);
    else
        T = glm::cross(vec3(1.0f, 0.0f, 0.0f), N);
    T = glm::normalize(T);
    B = glm::cross(N, T);
}

inline vec3 SDF::max() const
{
    return scale + translation;
}

inline vec3 SDF::min() const
{
    return -scale + translation;
}

inline float SDF::distance(vec3 p) const
{
    p -= translation;
    //p = glm::orientate3(rotation) * p;
    p /= scale;

    switch(type)
    {
        default:
        case SDF_SPHERE: 
            return glm::length(p) - 1.0f;
        case SDF_BOX:
            p = glm::abs(p) - 1.0f;
            return glm::min(glm::max(p.x, glm::max(p.y, p.z)), 0.0f) + glm::length(glm::max(p, glm::vec3(0.0f)));
    }
    return 1000.0f;
}

inline float SDF::blend(float a, float b) const
{
    switch(blend_type)
    {
        default:
        case SDF_UNION: 
            return a < b ? a : b;
        case SDF_DIFF:
            b = -b;
            return a < b ? b : a;
        case SDF_INTER:
            return a < b ? b : a;
        case SDF_S_UNION:
        {
            const float e = glm::max(smoothness - glm::abs(a - b), 0.0f);
            const float dis = glm::min(a, b) - e * e * 0.25f / smoothness;
            return dis;
        }
        case SDF_S_DIFF:
        {
            b = -b;
            const float e = glm::max(smoothness - glm::abs(a - b), 0.0f);
            const float dis = glm::max(a, b) - e * e * 0.25f / smoothness;
            return dis;
        }
        case SDF_S_INTER:
        {
            const float e = glm::max(smoothness - glm::abs(a - b), 0.0f);
            const float dis = glm::max(a, b) - e * e * 0.25f / smoothness;
            return dis;
        }
    }
    return a;
}

inline float SDFDis(const SDFList& sdfs, const SDFIndices& indices, const vec3 p)
{
    float dis = 1000.0f;
    for(const u16 i : indices)
    {
        const SDF& sdf = sdfs[i];
        dis = sdf.blend(dis, sdf.distance(p));
    }
    return dis;
}

inline float SDFDis(const SDFList& sdfs, const vec3 p)
{
    float dis = 1000.0f;
    for(const SDF& sdf : sdfs)
    {
        dis = sdf.blend(dis, sdf.distance(p));
    }
    return dis;
}

inline vec3 SDFNorm(const SDFList& sdfs, const SDFIndices& indices, const vec3 p)
{
    const float e = 0.001f;
    return normalize(vec3(
        SDFDis(sdfs, indices, p + vec3(e, 0.0f, 0.0f)) - SDFDis(sdfs, indices, p - vec3(e, 0.0f, 0.0f)),
        SDFDis(sdfs, indices, p + vec3(0.0f, e, 0.0f)) - SDFDis(sdfs, indices, p - vec3(0.0f, e, 0.0f)),
        SDFDis(sdfs, indices, p + vec3(0.0f, 0.0f, e)) - SDFDis(sdfs, indices, p - vec3(0.0f, 0.0f, e))
    ));
}

inline vec3 SDFNorm(const SDFList& sdfs, const vec3 p)
{
    const float e = 0.001f;
    return normalize(vec3(
        SDFDis(sdfs, p + vec3(e, 0.0f, 0.0f)) - SDFDis(sdfs, p - vec3(e, 0.0f, 0.0f)),
        SDFDis(sdfs, p + vec3(0.0f, e, 0.0f)) - SDFDis(sdfs, p - vec3(0.0f, e, 0.0f)),
        SDFDis(sdfs, p + vec3(0.0f, 0.0f, e)) - SDFDis(sdfs, p - vec3(0.0f, 0.0f, e))
    ));
}

inline Material SDFMaterial(const SDFList& sdfs, const SDFIndices& indices, const vec3 p)
{
    if(indices.count() == 1)
    {
        return sdfs[indices[0]].material;
    }

    Material A, B;
    float disA = 1000.0f, disB = 1000.0f;
    for(const u16 i : indices)
    {
        const SDF& sdf = sdfs[i];
        const float dis = sdf.distance(p);
        if(glm::abs(dis) < disA)
        {
            disB = disA;
            disA = dis;
            B = A;
            A = sdf.material;
        }
    }

    Material retMat;
    const float alpha = 0.5f * (disA / disB);
    retMat.setColor(glm::mix(A.getColor(), B.getColor(), alpha));
    retMat.setRoughness(glm::mix(A.getRoughness(), B.getRoughness(), alpha));
    retMat.setMetalness(glm::mix(A.getMetalness(), B.getMetalness(), alpha));

    return retMat;
}

inline float SDFAO(const SDFList& sdfs, const vec3 p, const vec3 N)
{
    float ao = 0.0f;
    const s32 num_steps = 16;
    float len = 0.01f;
    vec3 T, B;
    findBasis(N, T, B);
    const float base = SDFDis(sdfs, p);
    for(s32 i = 0; i < num_steps; ++i)
    {
        const vec3 pos = p + len * N;
        if(SDFDis(sdfs, pos + T * len) < base)
        {
            ao += 1.0f;
        }
        if(SDFDis(sdfs, pos - T * len) < base)
        {
            ao += 1.0f;
        }
        if(SDFDis(sdfs, pos + B * len) < base)
        {
            ao += 1.0f;
        }
        if(SDFDis(sdfs, pos - B * len) < base)
        {
            ao += 1.0f;
        }
        len *= 2.0f;
    }

    ao /= float(num_steps * 4);

    return glm::sqrt(ao);
}
