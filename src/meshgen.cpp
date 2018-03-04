#include "meshgen.h"

#include "sdf.h"
#include "quickhull.h"
#include "vertexbuffer.h"
#include <thread>
#include <mutex>
#include <atomic>

vec3 vecMin(vec3 a, vec3 b)
{
    return vec3(
        a.x < b.x ? a.x : b.x,
        a.y < b.y ? a.y : b.y,
        a.z < b.z ? a.z : b.z
    );
}

vec3 vecMax(vec3 a, vec3 b)
{
    return vec3(
        a.x > b.x ? a.x : b.x,
        a.y > b.y ? a.y : b.y,
        a.z > b.z ? a.z : b.z
    );
}

void CreateMesh(Geometry& output, const SDFList& sdfs, unsigned maxDepth)
{
    const float cornerRatio = glm::sqrt(3.0f);

    struct Task
    {
        glm::vec3 center;
        float radius = 0.0f;
        SDFIndices indices;
        unsigned depth = 0;
        void assume(Task& other)
        {
            center = other.center;
            radius = other.radius;
            indices.assume(other.indices);
            depth = other.depth;
        }
    };
    Vector<qh_vertex> vertices;
    Vector<Task> tasks;
    std::mutex taskMutex;
    std::mutex vertMutex;

    {
        vec3 lo = vec3( 999999.0f);
        vec3 hi = vec3(-999999.0f);
        Task& rootTask = tasks.grow();
        const unsigned num_sdfs = sdfs.count();
        rootTask.indices.resize(num_sdfs);
        for(unsigned i = 0; i < num_sdfs; ++i)
        {
            rootTask.indices[i] = i;
            lo = vecMin(lo, sdfs[i].min());
            hi = vecMax(hi, sdfs[i].max());
        }

        rootTask.center = 0.5f * lo + 0.5f * hi;
        rootTask.radius = glm::distance(lo, hi) * 0.5f;
    }

    auto DoTask = [&]()
    {
        Task task;
        taskMutex.lock();
        if(tasks.count())
        {
            task.assume(tasks.pop());
        }
        taskMutex.unlock();

        if(!task.indices.count())
            return;

        if(task.depth == maxDepth)
        {
            vec3 pt = task.center;
            {
                const vec3 N = SDFNorm(sdfs, task.indices, pt);
                const float e = 0.001f;
                float dis = SDFDis(sdfs, task.indices, pt);
                while(glm::abs(dis) > e)
                {
                    pt -= N * dis;
                    dis = SDFDis(sdfs, task.indices, pt);
                }
            }

            qh_vertex_t vert;
            vert.x = pt.x;
            vert.y = pt.y;
            vert.z = pt.z;
            
            vertMutex.lock();
            vertices.grow() = vert;
            vertMutex.unlock();
        }
        else
        {
            for(unsigned i = 0; i < 8; ++i)
            {
                Task child;
                child.center = task.center;
                child.radius = task.radius * 0.5f;
                child.depth = task.depth + 1;
                child.center.x += i & 1 ? -child.radius : child.radius;
                child.center.y += i & 2 ? -child.radius : child.radius;
                child.center.z += i & 4 ? -child.radius : child.radius;
                child.indices.clear();

                for(unsigned j : task.indices)
                {
                    if(sdfs[j].distance(child.center) < child.radius)
                    {
                        child.indices.grow() = j;
                    }
                }

                if(child.indices.count())
                {
                    taskMutex.lock();
                    tasks.grow().assume(child);
                    taskMutex.unlock();
                }
            }
        }
    };

    auto ContinuousTask = [&]()
    {
        const int max_spins = 25;
        int spins = 0;
        while(spins < max_spins)
        {
            while(tasks.count())
            {
                DoTask();
            }
            ++spins;
        }
    };

    const int num_threads = 8;
    while(tasks.count() < num_threads)
    {
        DoTask();
    }

    std::thread threads[num_threads];
    for(std::thread& t : threads)
    {
        t = std::thread(ContinuousTask);
    }

    for(std::thread& t : threads)
    {
        t.join();
    }
    
    const qh_mesh_t mesh = qh_quickhull3d(vertices.begin(), vertices.count());

    VertexBuffer& vb = output.m_vb;
    IndexBuffer& ib = output.m_ib;
    vb.resize(mesh.nvertices);
    ib.resize(mesh.nindices);
    for(unsigned i = 0; i < mesh.nindices; ++i)
    {
        ib[i] = mesh.indices[i];
    }

    std::atomic<unsigned> curVert;
    curVert = 0;

    auto DoVert = [&]()
    {
        while(true)
        {
            const unsigned i = curVert++;
            if(i >= mesh.nvertices)
                break;

            const qh_vertex_t& vert = mesh.vertices[i];
            vec3 pt(vert.x, vert.y, vert.z);
            vec3 N = SDFNorm(sdfs, pt);

            vec2 uv;
            {
                const vec3 d = glm::abs(N);
                const float a = glm::max(glm::max(d.x, d.y), d.z);
                if(a == d.x)
                    uv = vec2(pt.z, pt.y);
                else if(a == d.y)
                    uv = vec2(pt.x, pt.z);
                else
                    uv = vec2(pt.x, pt.y);
            }

            vb[i] = {
                vec4(pt.x, pt.y, pt.z, uv.x),
                vec4(N.x, N.y, N.z, uv.y)
            };
        }
    };

    for(std::thread& t : threads)
    {
        t = std::thread(DoVert);
    }
    
    for(std::thread& t : threads)
    {
        t.join();
    }

    qh_free_mesh(mesh);
}