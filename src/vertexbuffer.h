#pragma once

#include "linmath.h"
#include "array.h"

struct Vertex 
{
    vec4 position;     // w = uv.x
    vec4 normal;       // w = uv.y
};

typedef Vector<Vertex> VertexBuffer;
typedef Vector<unsigned> IndexBuffer;

struct Geometry
{
    VertexBuffer m_vb;
    IndexBuffer m_ib;
};