#pragma once

#include "ints.h"
#include "asserts.h"
#include "linmath.h"
#include "sdf.h"

#define RF_CAP 8

struct GLProgram;

struct RasterField
{
    float m_field[RF_CAP][RF_CAP][RF_CAP];
    vec3 m_translation;
    vec3 m_scale;
    RasterField()
    {
        u32* p = (u32*)m_field;
        const u32 count = RF_CAP * RF_CAP * RF_CAP;
        for(u32 i = 0u; i < count; ++i)
        {
            p[i] = 0u;
        }
    }
    void updateColumn(const SDFList& sdfs, const u32 column);
    void update(const SDFList& sdfs, const u32 num_threads=8);
};

void InitRasterFields();
void DrawRasterField(const RasterField& field, GLProgram& prog);
