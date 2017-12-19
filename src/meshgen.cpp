
#include "meshgen.h"
#include "assert.h"
#include <glm/gtx/euler_angles.hpp>
#include <thread>
#include <mutex>

using namespace glm;

#define Bit(x) (1u << (x))

void findBasis(vec3 N, vec3& T, vec3& B)
{
    if(glm::abs(N.x) > 0.001f)
        T = cross(vec3(0.0f, 1.0f, 0.0f), N);
    else
        T = cross(vec3(1.0f, 0.0f, 0.0f), N);
    T = normalize(T);
    B = cross(N, T);
}

float randf(u32& f) 
{
    f = (f ^ 61) ^ (f >> 16);
    f *= 9;
    f = f ^ (f >> 4);
    f *= 0x27d4eb2d;
    f = f ^ (f >> 15);
    return glm::fract(float(f) * 2.3283064e-10f);
}

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
    return 1000.0f;
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
    float dis = 1000.0f;
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

    if(disB >= 10.0f)
    {
        return A;
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
    return 0.0f;
    static u32 s = 521985231;
    float ao = 0.0f;
    const s32 num_steps = 4;
    for(s32 i = 0; i < num_steps; ++i)
    {
        const vec3 pos = p + N + vec3(randf(s), randf(s), randf(s)) * 2.0f - 1.0f;
        if(SDFDis(sdfs, indices, pos) < 0.0f)
        {
            ao += 1.0f;
        }
    }

    ao /= float(num_steps);

    return ao;
}

#define Bits(a, b, c, d) ((1 << a) | (1 << b) | (1 << c) | (1 << d))
#define CodeRight  Bits(4, 5, 6, 7)
#define CodeLeft   Bits(0, 1, 2, 3)
#define CodeUp     Bits(2, 3, 6, 7)
#define CodeDown   Bits(0, 1, 4, 5)
#define CodeFront  Bits(0, 2, 4, 6)
#define CodeBack   Bits(1, 3, 5, 7)
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

struct FaceIndices
{
    u32 indices[NumFaces];
};

#define ConsFaceIndices(a, b, c, d, e, f) { a, b, c, d, e, f }

const FaceIndices g_faces[NumFaces] = {
    ConsFaceIndices(7, 4, 6, 7, 4, 5),
    ConsFaceIndices(3, 2, 0, 3, 0, 1),
    ConsFaceIndices(6, 2, 3, 6, 3, 7),
    ConsFaceIndices(1, 4, 5, 1, 0, 4),
    ConsFaceIndices(2, 4, 0, 2, 6, 4),
    ConsFaceIndices(3, 5, 7, 3, 1, 5)
};

// pts[0] = st.center;
// pts[1] = st.center + vec3(0.0f,      0.0f,    offset);  
// pts[2] = st.center + vec3(0.0f,      offset,  0.0f);    
// pts[3] = st.center + vec3(0.0f,      offset,  offset);  
// pts[4] = st.center + vec3(offset,    0.0f,    0.0f);    
// pts[5] = st.center + vec3(offset,    0.0f,    offset);  
// pts[6] = st.center + vec3(offset,    offset,  0.0f);    
// pts[7] = st.center + vec3(offset,    offset,  offset);  

void GenerateMesh(MeshTask& task)
{
    task.geom.vertices.clear();

    struct SubTask
    {
        Vector<u16> indices;
        glm::vec3 center;
        float radius = 0.0f;
        u32 depth = 0;

        float qlen(){ return 1.732051f * radius; }
    };

    Vector<SubTask> subtasks;
    {
        SubTask& st = subtasks.grow();
        st.center = task.center;
        st.radius = task.radius;
        st.depth = 0;

        const float qlen = st.qlen();
        const u16 numSdfs = task.sdfs.count();
        for(u16 i = 0; i < numSdfs; ++i)
        {
            const float dis = task.sdfs[i].distance(st.center);
            if(dis < qlen)
            {
                st.indices.grow() = i;
            }
        }
    }

    const s32 num_threads = 16;
    std::mutex subtaskLock;
    std::mutex vertexLock;
    std::thread threads[num_threads];

    auto ThreadTask = [&]()
    {
        subtaskLock.lock();
        if(!subtasks.count())
        {
            subtaskLock.unlock();
            return;
        }
        SubTask st = subtasks.pop();
        subtaskLock.unlock();

        if(st.indices.count() == 0)
            return;

        if(st.depth == task.max_depth)
        {
            vec3 pts[8];
            vec3 Ns[8];
            u32 code = 0;
            {
                const float offset = st.radius * 2.0f;
                pts[0] = st.center;
                pts[1] = st.center + vec3(0.0f,      0.0f,    offset);  
                pts[2] = st.center + vec3(0.0f,      offset,  0.0f);    
                pts[3] = st.center + vec3(0.0f,      offset,  offset);  
                pts[4] = st.center + vec3(offset,    0.0f,    0.0f);    
                pts[5] = st.center + vec3(offset,    0.0f,    offset);  
                pts[6] = st.center + vec3(offset,    offset,  0.0f);    
                pts[7] = st.center + vec3(offset,    offset,  offset);  

                const float qlen = st.qlen();
                for(u32 i = 0; i < 8; ++i)
                {
                    const bool b = glm::abs(SDFDis(task.sdfs, st.indices, pts[i])) < qlen;
                    if(b)
                    {
                        code |= 1 << i;
                        Ns[i] = SDFNorm(task.sdfs, st.indices, pts[i]);
                        pts[i] -= Ns[i] * SDFDis(task.sdfs, st.indices, pts[i]);
                    }
                }
            }
            Array<Vertex, 3 * 2 * 6> outVerts;

            for(u32 i = 0; i < NumFaces; ++i)
            {
                if((code & g_codes[i]) == g_codes[i])
                {
                    for(const u32 idx : g_faces[i].indices)
                    {
                        Vertex& vert = outVerts.grow();
                        vert.setPosition(pts[idx]);
                        vert.setNormal(Ns[idx]);
                        const Material mat = SDFMaterial(task.sdfs, st.indices, pts[idx]);
                        vert.setColor(mat.getColor());
                        const float roughness = mat.getRoughness();
                        const float metalness = mat.getMetalness();
                        const float ao = SDFAO(task.sdfs, st.indices, pts[idx], Ns[idx]);
                        vert.setMaterial(glm::vec3(roughness, metalness, ao));
                    }
                }
            }

            vertexLock.lock();
            for(const Vertex& vert : outVerts)
            {
                task.geom.vertices.grow() = vert;
            }
            vertexLock.unlock();
        }
        else
        {
            const float nlen = st.radius * 0.5f;
            for(u32 i = 0; i < 8; ++i)
            {
                vec3 nc = st.center;
                nc.x += (i & 1) ? -nlen : nlen;
                nc.y += (i & 2) ? -nlen : nlen;
                nc.z += (i & 4) ? -nlen : nlen;

                SubTask child;
                child.center = nc;
                child.radius = nlen;
                child.depth = st.depth + 1;

                const float qlen = child.qlen();
                for(const u16 idx : st.indices)
                {
                    const float dis = task.sdfs[idx].distance(child.center);
                    if(dis < qlen)
                    {
                        child.indices.grow() = idx;
                    }
                }

                if(child.indices.count())
                {
                    subtaskLock.lock();
                    subtasks.grow() = child;
                    subtaskLock.unlock();
                }
            }
        }
    };

    auto ContinuousTask = [&]()
    {
        while(subtasks.count())
        {
            ThreadTask();
        }
    };

    while(subtasks.count() < num_threads)
    {
        ThreadTask();
    }

    while(subtasks.count())
    {
        for(s32 i = 0; i < num_threads; ++i)
        {
            threads[i] = std::thread(ContinuousTask);
        }

        for(s32 i = 0; i < num_threads; ++i)
        {
            threads[i].join();
        }
    }
}
