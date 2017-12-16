
#include "meshgen.h"
#include "assert.h"
#include <glm/gtx/euler_angles.hpp>

using namespace glm;

MeshTaskContext g_meshTaskContexts[MeshTaskContext::ContextCapacity];

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
    p = glm::orientate3(rotation) * p;
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

float SDFDis(const SDFList& sdfs, const vec3 p)
{
    float dis = SDF_BigDistance;
    for(const SDF& sdf : sdfs)
    {
        dis = sdf.blend(dis, sdf.distance(p));
    }
    return dis;
}

vec3 SDFNorm(const SDFList& sdfs, const vec3 p)
{
    const float e = 0.001f;
    return normalize(vec3(
        SDFDis(sdfs, p + vec3(e, 0.0f, 0.0f)) - SDFDis(sdfs, p - vec3(e, 0.0f, 0.0f)),
        SDFDis(sdfs, p + vec3(0.0f, e, 0.0f)) - SDFDis(sdfs, p - vec3(0.0f, e, 0.0f)),
        SDFDis(sdfs, p + vec3(0.0f, 0.0f, e)) - SDFDis(sdfs, p - vec3(0.0f, 0.0f, e))
    ));
}

Material SDFMaterial(const SDFList& sdfs, const vec3 p)
{
    Material A, B;
    float disA = 1000.0f, disB = 1000.0f;
    for(const SDF& sdf : sdfs)
    {
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

float SDFAO(const SDFList& sdfs, const vec3 p, const vec3 N)
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
        if(SDFDis(sdfs, pos + T * len) < 0.0f)
        {
            ao += 1.0f;
        }
        if(SDFDis(sdfs, pos - T * len) < 0.0f)
        {
            ao += 1.0f;
        }
        if(SDFDis(sdfs, pos + B * len) < 0.0f)
        {
            ao += 1.0f;
        }
        if(SDFDis(sdfs, pos - B * len) < 0.0f)
        {
            ao += 1.0f;
        }
        len *= 5.0f;
    }

    ao /= float(num_steps * samples_per_step);

    return ao;
}

void GenerateMesh(MeshTask& task, u32 thread_id)
{
    assert(task.pitch > 2 && task.pitch < MeshTaskContext::GridCapacity);
    assert(thread_id >= 0 && thread_id < MeshTaskContext::ContextCapacity);
    task.geom.vertices.clear();
    task.geom.indices.clear();

    MeshTaskContext& ctx = g_meshTaskContexts[thread_id];
    ctx.indices.clear();

    const u32 pitch = task.pitch;
    const float inv_pitch = 1.0f / float(pitch);
    {
        const vec3 dv = task.bounds.span() * inv_pitch;
        for(u32 x = 0; x <= pitch; ++x)
        for(u32 y = 0; y <= pitch; ++y)
        for(u32 z = 0; z <= pitch; ++z)
        {
            vec3 p = task.bounds.lo + dv * vec3(float(x), float(y), float(z));
            float dis = SDFDis(task.sdfs, p);
            ctx.dis[x][y][z] = glm::abs(dis) < inv_pitch;
            if(ctx.dis[x][y][z])
            {
                const vec3 N = SDFNorm(task.sdfs, p);
                for(u32 i = 0; i < 10; ++i)
                {
                    p -= N * dis;
                    dis = SDFDis(task.sdfs, p);
                }
                ctx.grid[x][y][z] = p;
            }
        }
    }
    
    for(u32 x = 0; x < pitch; ++x)
    for(u32 y = 0; y < pitch; ++y)
    for(u32 z = 0; z < pitch; ++z)
    {
        if(!ctx.dis[x][y][z])
            continue;
        
        const uvec3 ip(x, y, z);
        u32 code = 0;
        for(u32 v = 0; v < NumVerts; ++v)
        {
            const uvec3 pi = ip + g_cube[v];
            code |= ctx.dis[pi.x][pi.y][pi.z] << v;
        }

        const u32 numIndices = ctx.indices.count();
        for(u32 c = 0; c < NumFaces; ++c)
        {
            if((g_codes[c] & code) == g_codes[c])
            {
                const uvec3* pFace = g_faces[c].indices;
                for(u32 f = 0; f < IndicesPerFace; ++f)
                {
                    ctx.indices.grow() = ip + pFace[f];
                }
            }
        }

        if(ctx.indices.count() != numIndices)
        {
            ctx.idx[x][y][z] = task.geom.vertices.count();
            Vertex& vert = task.geom.vertices.grow();
            vert.setPosition(ctx.grid[x][y][z]);
            const vec3 N = SDFNorm(task.sdfs, ctx.grid[x][y][z]);
            vert.setNormal(N);
            const Material mat = SDFMaterial(task.sdfs, ctx.grid[x][y][z]);
            vert.red = mat.red; vert.green = mat.green; vert.blue = mat.blue;
            vert.roughness = mat.roughness;
            vert.metalness = mat.metalness;
            vert.setAmbientOcclusion(SDFAO(task.sdfs, ctx.grid[x][y][z], N));
        }
    }

    const s32 numIndices = ctx.indices.count();
    task.geom.indices.reserve(numIndices);
    for(s32 i = 0; i < numIndices; ++i)
    {
        task.geom.indices.append() = ctx.idx[ctx.indices[i].x][ctx.indices[i].y][ctx.indices[i].z];
    }
}
