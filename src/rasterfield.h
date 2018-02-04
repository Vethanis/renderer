#pragma once

#include "ints.h"
#include "asserts.h"
#include "linmath.h"
#include "sdf.h"

#define RF_CAP 64
#define RASTER_FIELD_BINDING 9

struct GLProgram;

struct RasterField
{
    float m_field[RF_CAP][RF_CAP][RF_CAP];
    vec3 m_translation;
    vec3 m_scale;
    RasterField()
    {
        m_translation = vec3(0.0f);
        m_scale = vec3(1.0f);
        float* p = &m_field[0][0][0];
        const u32 count = RF_CAP * RF_CAP * RF_CAP;
        for(u32 i = 0; i < count; ++i)
        {
            p[i] = 0.0f;
        }
    }
    void updateColumn(const SDFList& sdfs, const u32 column);
    void update(const SDFList& sdfs, const u32 num_threads=8);
};

void InitRasterFields();
void DrawRasterField(const RasterField& field, GLProgram& prog);
