
#include "meshgen.h"

#if MESH_GEN_ENABLED

#include "asserts.h"
#include <glm/gtx/euler_angles.hpp>
#include <thread>
#include <mutex>

using namespace glm;

struct GridCell
{
    glm::vec3 pts[8];
    float vals[8];

    glm::vec3 interp(const float iso, const u32 i, const u32 j) const
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

u32 HandleTet(const GridCell& cell, glm::vec3* tris, const u32* ind, const float iso)
{
    u32 num_tri = 0;
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

struct SubTask
{
    Vector<u16> indices;
    glm::vec3 center;
    float radius = 0.0f;
    u32 depth = 0;

    float qlen(){ return 1.732052f * radius; }
};

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

    const u32 indices[6][4] = 
    {
        { 0, 2, 3, 7 },
        { 0, 2, 6, 7 },
        { 0, 4, 6, 7 },
        { 0, 6, 1, 2 },
        { 0, 6, 1, 4 },
        { 5, 6, 1, 4 }
    };

    Array<Vertex, 36> vertices;
    for(u32 i = 0; i < 6; ++i)
    {
        vec3 tris[6];
        const u32 num_verts = 3 * HandleTet(cell, tris, indices[i], iso);
        for(u32 j = 0; j < num_verts; ++j)
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

void MakePts(const SDFList& sdfs, SubTask& st, Vector<Vertex>& outVerts, std::mutex& mut)
{
    vec3 aN = SDFNorm(sdfs, st.indices, st.center);
    {
        const vec3 axN = glm::abs(axN);
        const float mc = glm::max(aN.x, glm::max(aN.y, aN.z));
        if(mc == aN.x)
        {
            aN = vec3(1.0f, 0.0f, 0.0f) * glm::sign(aN.x);
        }
        else if(mc == aN.y)
        {
            aN = vec3(0.0f, 1.0f, 0.0f) * glm::sign(aN.y);
        }
        else
        {
            aN = vec3(0.0f, 0.0f, 1.0f) * glm::sign(aN.z);
        }
    }

    vec3 pt = st.center;
    float travel = 0.0f;
    for(s32 i = 0; i < 10; ++i)
    {
        const float dis = SDFDis(sdfs, st.indices, pt);
        travel += glm::abs(dis);
        if(travel > st.radius)
            break;
        
        pt -= dis * aN;
    }

    mut.lock();
    Vertex& vert = outVerts.grow();
    mut.unlock();

    const vec3 N = SDFNorm(sdfs, st.indices, pt);
    const Material mat = SDFMaterial(sdfs, st.indices, pt);
    const float ao = SDFAO(sdfs, pt, N);
    const float roughness = mat.getRoughness();
    const float metalness = mat.getMetalness();
    vert.setPosition(pt);
    vert.setNormal(N);
    vert.setColor(mat.getColor());
    vert.setMaterial(glm::vec3(roughness, metalness, ao));
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
            //MakeTris(task.sdfs, st, task.geom.vertices, vertexLock, 0.0f);
            MakePts(task.sdfs, st, task.geom.vertices, vertexLock);
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
                    if(glm::abs(dis) < qlen)
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

    while(subtasks.count() < num_threads && subtasks.count())
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

void GenMeshTest(MeshTask& task)
{
    for(u32 id = 0; id < 256; ++id)
    {
        GridCell cell;
        const u32 ux = id % 8;
        const u32 uy = (id / 64);
        const u32 uz = (id / 8) % 8;
        const vec3 base = vec3(float(ux), float(uy), float(uz)) * 2.0f;
        const float width = 0.5f;

        for(u32 p = 0; p < 8; ++p)
        {
            vec3 pt = base;
            pt.x += (p & 1) ? -width : width;
            pt.y += (p & 2) ? -width : width;
            pt.z += (p & 4) ? -width : width;
            cell.pts[p] = pt;
            const float val = (id & (1 << p)) ? -0.5f : 1.0f;
            cell.vals[p] = val;

            printf("%u: %3.2f, %3.2f, %3.2f: %3.2f\n", id, pt.x, pt.y, pt.z, val);
        }
        
        const u32 indices[6][4] = 
        {
            { 0, 2, 3, 7 },
            { 0, 2, 6, 7 },
            { 0, 4, 6, 7 },
            { 0, 6, 1, 2 },
            { 0, 6, 1, 4 },
            { 5, 6, 1, 4 }
        };

        for(u32 i = 0; i < 6; ++i)
        {
            vec3 tris[6];
            const u32 num_verts = 3 * HandleTet(cell, tris, indices[i], 0.0f);
            vec3 Ns[2];
            Ns[0] = normalize(cross(tris[1]-tris[0], tris[2]-tris[0]));
            Ns[1] = normalize(cross(tris[4]-tris[3], tris[5]-tris[3]));
            for(u32 j = 0; j < num_verts; ++j)
            {
                Vertex& vert = task.geom.vertices.grow();
                const vec3 N = j >= 3 ? Ns[0] : Ns[1];
                const float ao = 0.0f;
                const float roughness = 0.5f;
                const float metalness = 0.0f;
                vert.setPosition(tris[j]);
                vert.setNormal(N);
                vert.setColor(vec3(1.0f, 0.0f, 0.0f));
                vert.setMaterial(glm::vec3(roughness, metalness, ao));
            }
        }
    }
}

#endif // MESH_GEN_ENABLED