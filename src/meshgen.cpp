
#include "meshgen.h"
#include "assert.h"
#include <glm/gtx/euler_angles.hpp>
#include <thread>
#include <mutex>

using namespace glm;

#define Bit(x) (1u << (x))
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
    ConsFaceIndices(6, 7, 4, 7, 5, 4),
    ConsFaceIndices(3, 2, 1, 2, 0, 1),
    ConsFaceIndices(6, 2, 7, 2, 3, 7),
    ConsFaceIndices(5, 1, 4, 1, 0, 4),
    ConsFaceIndices(2, 6, 0, 6, 4, 0),
    ConsFaceIndices(7, 3, 5, 3, 1, 5)
};

struct SubTask
{
    Vector<u16> indices;
    glm::vec3 center;
    float radius = 0.0f;
    u32 depth = 0;

    float qlen(){ return 1.732051f * radius; }
};

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

float SDFDis(const SDFList& sdfs, const vec3 p)
{
    float dis = 1000.0f;
    for(const SDF& sdf : sdfs)
    {
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

float SDFAO(const SDFList& sdfs, const vec3 p, const vec3 N)
{
    static u32 s = 521985231;
    float ao = 0.0f;
    const s32 num_steps = 8;
    float len = 0.01f;
    vec3 T, B;
    findBasis(N, T, B);
    for(s32 i = 0; i < num_steps; ++i)
    {
        const vec3 pos = p + len * N;
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
        len *= 2.0f;
    }

    ao /= float(num_steps * 4.0f);

    return ao;
}

struct GridCell
{
    glm::vec3 pts[8];
    float vals[8];

    glm::vec3 interp(const float iso, const s32 i, const s32 j) const
    {
        if(glm::abs(iso - vals[i]) < 0.00001f)
        {
            return pts[i];
        }
        if(glm::abs(iso - vals[j]) < 0.00001f)
        {
            return pts[j];
        }
        if(glm::abs(vals[i] - vals[j]) < 0.00001f)
        {
            return pts[i];
        }
        const float alpha = (iso - vals[i]) / (vals[j] - vals[i]);
        return glm::vec3(
            pts[i].x + alpha * (pts[j].x - pts[i].x),
            pts[i].y + alpha * (pts[j].y - pts[i].y),
            pts[i].z + alpha * (pts[j].z - pts[i].z)
        );
    }
};

s32 HandleTet(const GridCell& cell, glm::vec3* tris, const s32* ind, const float iso)
{
    s32 num_tri = 0;
    u32 code = 0;

    if(cell.vals[ind[0]] < iso) code |= 1;
    if(cell.vals[ind[1]] < iso) code |= 2;
    if(cell.vals[ind[2]] < iso) code |= 4;
    if(cell.vals[ind[3]] < iso) code |= 8;

   switch (code) 
   {
    case 0x00:
    case 0x0F:
        break;
    case 0x0E:
    case 0x01:
        tris[0] = cell.interp(iso, ind[0], ind[1]);
        tris[1] = cell.interp(iso, ind[0], ind[2]);
        tris[2] = cell.interp(iso, ind[0], ind[3]);
        num_tri++;
        break;
    case 0x0D:
    case 0x02:
        tris[0] = cell.interp(iso, ind[1], ind[0]);
        tris[1] = cell.interp(iso, ind[1], ind[3]);
        tris[2] = cell.interp(iso, ind[1], ind[2]);
        num_tri++;
        break;
    case 0x0C:
    case 0x03:
        tris[0] = cell.interp(iso, ind[0], ind[3]);
        tris[1] = cell.interp(iso, ind[0], ind[2]);
        tris[2] = cell.interp(iso, ind[1], ind[3]);
        num_tri++;
        tris[3] = tris[2];
        tris[4] = cell.interp(iso, ind[1], ind[2]);
        tris[5] = tris[1];
        num_tri++;
        break;
    case 0x0B:
    case 0x04:
        tris[0] = cell.interp(iso, ind[2], ind[0]);
        tris[1] = cell.interp(iso, ind[2], ind[1]);
        tris[2] = cell.interp(iso, ind[2], ind[3]);
        num_tri++;
        break;
    case 0x0A:
    case 0x05:
        tris[0] = cell.interp(iso, ind[0], ind[1]);
        tris[1] = cell.interp(iso, ind[2], ind[3]);
        tris[2] = cell.interp(iso, ind[0], ind[3]);
        num_tri++;
        tris[3] = tris[0];
        tris[4] = cell.interp(iso, ind[1], ind[2]);
        tris[5] = tris[1];
        num_tri++;
        break;
    case 0x09:
    case 0x06:
        tris[0] = cell.interp(iso, ind[0], ind[1]);
        tris[1] = cell.interp(iso, ind[1], ind[3]);
        tris[2] = cell.interp(iso, ind[2], ind[3]);
        num_tri++;
        tris[3] = tris[0];
        tris[4] = cell.interp(iso, ind[0], ind[2]);
        tris[5] = tris[2];
        num_tri++;
        break;
    case 0x07:
    case 0x08:
        tris[0] = cell.interp(iso, ind[3], ind[0]);
        tris[1] = cell.interp(iso, ind[3], ind[2]);
        tris[2] = cell.interp(iso, ind[3], ind[1]);
        num_tri++;
        break;
   }

   return num_tri;
}

void MakeTris(const SDFList& sdfs, SubTask& st, Vector<Vertex>& outVerts, std::mutex& mut, const float iso)
{
    GridCell cell;

    const float offset = st.radius * 2.0f;
    for(u32 i = 0; i < 8; ++i)
    {
        cell.pts[i] = st.center;
        cell.pts[i].x += (i & 1) ? -offset : offset;
        cell.pts[i].y += (i & 2) ? -offset : offset;
        cell.pts[i].z += (i & 4) ? -offset : offset;
        cell.vals[i] = SDFDis(sdfs, st.indices, cell.pts[i]);
    }

    const s32 indices[6][4] = 
    {
        { 0, 2, 3, 7 },
        { 0, 2, 6, 7 },
        { 0, 4, 6, 7 },
        { 0, 6, 1, 2 },
        { 0, 6, 1, 4 },
        { 5, 6, 1, 4 }
    };

    Array<Vertex, 36> vertices;
    for(s32 i = 0; i < 6; ++i)
    {
        vec3 tris[6];
        const s32 num_verts = 3 * HandleTet(cell, tris, indices[i], iso);
        for(s32 j = 0; j < num_verts; ++j)
        {
            Vertex& vert = vertices.grow();
            const vec3 N = SDFNorm(sdfs, st.indices, tris[j]);
            const Material mat = SDFMaterial(sdfs, st.indices, tris[j]);
            const float ao = SDFAO(sdfs, tris[j], N);
            const float roughness = mat.getRoughness();
            const float metalness = mat.getMetalness();
            vert.setPosition(tris[j]);
            vert.setNormal(N);
            vert.setColor(mat.getColor());
            vert.setMaterial(glm::vec3(roughness, metalness, ao));
        }
    }

    if(vertices.count())
    {
        mut.lock();
        for(const Vertex& vert : vertices)
        {
            outVerts.grow() = vert;
        }
        mut.unlock();
    }
}

void GenerateMesh(MeshTask& task)
{
    task.geom.vertices.clear();

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
            MakeTris(task.sdfs, st, task.geom.vertices, vertexLock, 0.0f);
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
