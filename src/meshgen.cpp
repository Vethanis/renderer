
#include "meshgen.h"
#include "assert.h"
#include <glm/gtx/euler_angles.hpp>

using namespace glm;

#define Bits(a, b, c, d) ((1 << a) | (1 << b) | (1 << c) | (1 << d))
#define CodeRight  Bits(0, 1, 2, 3)
#define CodeLeft   Bits(4, 5, 6, 7)
#define CodeUp     Bits(2, 3, 6, 7)
#define CodeDown   Bits(0, 1, 4, 5)
#define CodeFront  Bits(1, 3, 5, 7)
#define CodeBack   Bits(0, 2, 4, 6)
#define NumFaces  6
#define NumVerts  8
#define IndicesPerFace 6
#define SDF_BigDistance (float(1 << 22))

const u32 g_codes[NumFaces] = {
    CodeRight,
    CodeLeft,
    CodeUp,
    CodeDown,
    CodeFront,
    CodeBack,
};

const uvec3 g_cube[NumVerts] = {
    {0, 0, 0},
    {0, 0, 1},
    {0, 1, 0},
    {0, 1, 1},
    {1, 0, 0},
    {1, 0, 1},
    {1, 1, 0},
    {1, 1, 1}
};

struct FaceIndices
{
    uvec3 indices[NumFaces];
};

#define ConsFaceIndices(a, b, c, d, e, f) { g_cube[a], g_cube[b], g_cube[c], g_cube[d], g_cube[e], g_cube[f] }

const FaceIndices g_faces[NumFaces] = {
    ConsFaceIndices(1, 0, 3, 0, 2, 3),
    ConsFaceIndices(5, 7, 4, 7, 6, 4),
    ConsFaceIndices(3, 2, 6, 3, 6, 7),
    ConsFaceIndices(1, 4, 0, 1, 5, 4),
    ConsFaceIndices(6, 2, 4, 2, 0, 4),
    ConsFaceIndices(1, 3, 5, 3, 7, 5)
};

float SDF::distance(vec3 p) const
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
    return SDF_BigDistance;
}

float SDF::blend(float a, float b) const
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

float SDFDis(const SDFList& sdfs, const SDFIndices& indices, const vec3 p)
{
    float dis = SDF_BigDistance;
    for(const u16 i : indices)
    {
        const SDF& sdf = sdfs[i];
        dis = sdf.blend(dis, sdf.distance(p));
    }
    return dis;
}

vec3 SDFNorm(const SDFList& sdfs, const SDFIndices& indices, const vec3 p)
{
    const float e = 0.001f;
    return normalize(vec3(
        SDFDis(sdfs, indices, p + vec3(e, 0.0f, 0.0f)) - SDFDis(sdfs, indices, p - vec3(e, 0.0f, 0.0f)),
        SDFDis(sdfs, indices, p + vec3(0.0f, e, 0.0f)) - SDFDis(sdfs, indices, p - vec3(0.0f, e, 0.0f)),
        SDFDis(sdfs, indices, p + vec3(0.0f, 0.0f, e)) - SDFDis(sdfs, indices, p - vec3(0.0f, 0.0f, e))
    ));
}

Material SDFMaterial(const SDFList& sdfs, const SDFIndices& indices, const vec3 p)
{
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

float SDFAO(const SDFList& sdfs, const SDFIndices& indices, const vec3 p, const vec3 N)
{
    float ao = 0.0f;
    vec3 T, B;
    {
        if(glm::abs(N.x) > 0.001f)
            T = glm::cross(vec3(0.0f, 1.0f, 0.0f), N);
        else
            T = glm::cross(vec3(1.0f, 0.0f, 0.0f), N);

        T = glm::normalize(T);
        B = glm::cross(N, T);
    }
    float len = 0.01f;
    const s32 num_steps = 4;
    const s32 samples_per_step = 4;
    for(s32 i = 0; i < num_steps; ++i)
    {
        const vec3 pos = p + N * len;
        if(SDFDis(sdfs, indices, pos + T * len) < 0.0f)
        {
            ao += 1.0f;
        }
        if(SDFDis(sdfs, indices, pos - T * len) < 0.0f)
        {
            ao += 1.0f;
        }
        if(SDFDis(sdfs, indices, pos + B * len) < 0.0f)
        {
            ao += 1.0f;
        }
        if(SDFDis(sdfs, indices, pos - B * len) < 0.0f)
        {
            ao += 1.0f;
        }
        len *= 5.0f;
    }

    ao /= float(num_steps * samples_per_step);

    return ao;
}

void GenerateMesh(MeshTask& task)
{
    task.geom.vertices.clear();

    struct SubTask
    {
        Vector<u16> indices;
        glm::vec3 center;
        float radius = 0.0f;
        u32 depth = 0;
    };

    Vector<SubTask> subtasks;

    {
        const vec3 center = task.bounds.center();
        const float rad = task.bounds.cornerRadius();
        SubTask& st = subtasks.grow();
        st.center = center;
        st.radius = rad;
        st.depth = 0;

        const u16 numSdfs = task.sdfs.count();
        for(u16 i = 0; i < numSdfs; ++i)
        {
            const float dis = task.sdfs[i].distance(st.center);
            if(dis < st.radius)
            {
                st.indices.grow() = i;
            }
        }
    }

    while(subtasks.count())
    {
        const SubTask st = subtasks.pop();
        
        if(st.indices.count() == 0)
            continue;

        if(st.depth == task.max_depth)
        {
            const vec3 N = SDFNorm(task.sdfs, st.indices, st.center);
            const vec3 p = st.center - N * SDFDis(task.sdfs, st.indices, st.center);
            Vertex& vert = task.geom.vertices.grow();
            vert.setPosition(p);
            vert.setNormal(N);
            const Material mat = SDFMaterial(task.sdfs, st.indices, p);
            vert.red = mat.red; vert.green = mat.green; vert.blue = mat.blue;
            vert.roughness = mat.roughness;
            vert.metalness = mat.metalness;
            vert.setAmbientOcclusion(SDFAO(task.sdfs, st.indices, p, N));
        }
        else
        {
            const float nlen = st.radius * 0.5f;
            for(u8 i = 0; i < 8; ++i)
            {
                vec3 nc = st.center;
                nc.x += (i & 1) ? -nlen : nlen;
                nc.y += (i & 2) ? -nlen : nlen;
                nc.z += (i & 4) ? -nlen : nlen;

                SubTask& child = subtasks.grow();
                child.center = nc;
                child.radius = nlen;
                child.depth = st.depth + 1;

                for(u16 idx : st.indices)
                {
                    if(task.sdfs[idx].distance(child.center) < child.radius)
                    {
                        child.indices.grow() = idx;
                    }
                }
            }
        }
        
    }
}
