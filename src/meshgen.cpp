#include "meshgen.h"

#include "sdf.h"
#include "vertexbuffer.h"
#include "isosurface.h"
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
    VertexBuffer& vertices = output.m_vb;
    vertices.clear();
    output.m_ib.clear();
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

        hi += 0.01f;
        lo -= 0.01f;
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
            GRIDCELL g;
            const float radius = task.radius;
            g.p[0] = task.center + vec3(-radius);
            g.p[1] = task.center + vec3(radius, -radius, -radius);
            g.p[2] = task.center + vec3(radius, -radius, radius);
            g.p[3] = task.center + vec3(-radius, -radius, radius);
            g.p[4] = task.center + vec3(-radius, radius, -radius);
            g.p[5] = task.center + vec3(radius, radius, -radius);
            g.p[6] = task.center + vec3(radius, radius, radius);
            g.p[7] = task.center + vec3(-radius, radius, radius);
            
            for(unsigned i = 0; i < 8; ++i)
            {
                g.val[i] = SDFDis(sdfs, task.indices, g.p[i]);
            }

            Vector<vec3> localVerts;
            PolygoniseCell(g, 0.0f, localVerts);
            
            if(localVerts.count())
            {
                vertMutex.lock();
                for(const vec3& v : localVerts)
                {
                    vertices.grow() = { vec4(v.x, v.y, v.z, 1.0f) };
                }
                vertMutex.unlock();
            }
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
                    if(sdfs[j].distance(child.center) < child.radius * cornerRatio)
                    {
                        child.indices.grow() = j;
                    }
                }

                if(child.indices.count() && glm::abs(SDFDis(sdfs, child.indices, child.center)) < child.radius * cornerRatio)
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

    output.m_ib.reserve(vertices.count());
    for(int i = 0; i < vertices.count(); ++i)
    {
        output.m_ib.append() = i;
    }
}