
#include "myglheaders.h"
#include "rasterfield.h"
#include "mesh.h"
#include "glprogram.h"
#include "shared_uniform.h"
#include <thread>

u32 texHandle = 0;
Mesh mesh;

void InitRasterFields()
{
    {
        glGenTextures(1, &texHandle); DebugGL();
        glBindTexture(GL_TEXTURE_3D, texHandle); DebugGL();
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); DebugGL();
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); DebugGL();
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT); DebugGL();
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT); DebugGL();
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_REPEAT); DebugGL();
        glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, RF_CAP, RF_CAP, RF_CAP, 0, GL_RED, GL_FLOAT, nullptr); DebugGL();
    }

    const vec3 pts[8] = 
    {
        {-0.5f, -0.5f, -0.5f},
        {-0.5f, -0.5f, 0.5f},
        {-0.5f, 0.5f, -0.5f},
        {-0.5f, 0.5f, 0.5f},
        {0.5f, -0.5f, -0.5f},
        {0.5f, -0.5f, 0.5f},
        {0.5f, 0.5f, -0.5f},
        {0.5f, 0.5f, 0.5f}
    };
    #define Face(a, b, c, d) { pts[a] }, { pts[b] }, { pts[c] }, { pts[a] }, { pts[c] }, { pts[d] }
    const Vertex cube[3 * 2 * 6] = 
    {
        Face(6, 2, 0, 4),
        Face(3, 7, 5, 1),
        Face(2, 3, 1, 0),
        Face(7, 6, 4, 5),
        Face(7, 3, 2, 6),
        Face(1, 5, 4, 0)
    };
    #undef Face
    mesh.init();
    mesh.upload(cube, 3*2*6);
}

void DrawRasterField(const RasterField& field, GLProgram& prog)
{
    g_sharedUniforms.df_translation = vec4(field.m_translation.x, field.m_translation.y, field.m_translation.z, 1.0f / RF_CAP);
    g_sharedUniforms.df_scale = vec4(field.m_scale.x, field.m_scale.y, field.m_scale.z, g_sharedUniforms.df_scale.w);

    prog.bind3DTexture(RASTER_FIELD_BINDING, texHandle, "distance_field");
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32F, RF_CAP, RF_CAP, RF_CAP, 0, GL_RED, GL_FLOAT, field.m_field); DebugGL();

    mesh.draw();
}
    
void RasterField::updateColumn(const SDFList& sdfs, const u32 column)
{
    Assert(column < RF_CAP);
    const u32 ux = column;
    {
        for(u32 uy = 0; uy < RF_CAP; ++uy)
        {
            for(u32 uz = 0; uz < RF_CAP; ++uz)
            {
                const vec3 p = m_scale * (m_translation + vec3(float(ux), float(uy), float(uz)));
                m_field[ux][uy][uz] = SDFDis(sdfs, p);
            }
        }
    }
}

void RasterField::update(const SDFList& sdfs, const u32 num_threads)
{
    #if 0
    Assert(num_threads <= RF_CAP)
    std::thread threads[RF_CAP];
    for(u32 i = 0; i < num_threads; ++i)
    {
        threads[i] = std::thread(&RasterField::updateColumn, this, sdfs, i);
    }
    for(u32 i = 0; i < num_threads; ++i)
    {
        threads[i].join();
    }
    #endif
    for(u32 i = 0; i < RF_CAP; ++i)
    {
        updateColumn(sdfs, i);
    }
}