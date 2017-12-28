#pragma once

#define MESH_GEN_ENABLED 0

#if MESH_GEN_ENABLED

#include "vertexbuffer.h"
#include "sdfs.h"

struct MeshTask
{
    Geometry geom;
    SDFList sdfs;
    vec3 center;
    float radius = 1.0f;
    u32 max_depth = 5;

    float getPointSize()
    {
        return radius * 2.75f * 1000.0f * glm::pow(0.5f, float(max_depth));
    }
};

void GenerateMesh(MeshTask& task);

void GenMeshTest(MeshTask& task);

#endif // MESH_GEN_ENABLED