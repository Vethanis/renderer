#pragma once 

#include "twister.h"
#include "hashstring.h"
#include "glprogram.h"
#include "transform.h"
#include "mesh.h"
#include "directional_light.h"
#include "transform.h"
#include "rasterfield.h"

// ------------------------------------------------------------------------

#define PREPASS_ENABLED     1

#define DF_DIRECT           0
#define DF_INDIRECT         1
#define DF_NORMALS          2
#define DF_REFLECT          3
#define DF_UV               4
#define DF_DIRECT_CUBEMAP   5
#define DF_VIS_CUBEMAP      6
#define DF_VIS_REFRACT      7
#define DF_VIS_ROUGHNESS    8
#define DF_VIS_METALNESS    9
#define DF_GBUFF            10
#define DF_SKY              11
#define DF_VIS_TANGENTS     12
#define DF_VIS_BITANGENTS   13
#define DF_VIS_SUN_SHADOW_DEPTH 14
#define DF_VIS_VELOCITY     15
#define DF_VIS_SHADOW_BUFFER 16
#define DF_VIS_LDN 17
#define DF_VIS_AO 18

#define ODF_DEFAULT         0
#define ODF_SKY             1

#define TX_ALBEDO_CHANNEL   0
#define TX_MATERIAL_CHANNEL 1
#define TX_SUN_CHANNEL      2
#define TX_CUBEMAP_CHANNEL  3
#define TX_POSITION_CHANNEL 4
#define TX_NORMAL_CHANNEL   5

// ------------------------------------------------------------------------

struct RenderResource 
{
    RasterField m_field;

    void updateField(const SDFList& list){ m_field.update(list); }
    void draw(GLProgram& prog) const { DrawRasterField(m_field, prog); }
};

struct Renderables 
{
    TwArray<RenderResource, 64> resources;
    GLProgram zProg;
    GLProgram defProg;

    DirectionalLight m_light;

    void init();
    void deinit();
    void bindSun(GLProgram& prog, int channel = TX_SUN_CHANNEL){ m_light.bind(prog, channel); }
    void shadowPass(const Camera& cam);
    void depthPass(const glm::vec3& eye, const Transform& VP);
    void defDraw(const glm::vec3& eye, const Transform& VP, u32 dflag, s32 width, s32 height, u32 target = 0);
    u16 request(){ return resources.request(); }
    void release(u16 handle){ resources.remove(handle); }
    RenderResource& operator[](u16 i){ return resources[i]; }
};

extern Renderables g_Renderables;
