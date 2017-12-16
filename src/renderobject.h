#pragma once 

#include "common.h"
#include "twister.h"
#include "hashstring.h"
#include "glprogram.h"
#include "transform.h"
#include "mesh.h"
#include "directional_light.h"
#include "transform.h"

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
    Transform m_transform;
    Mesh mesh;

    void draw() const { mesh.draw(); }
    void init() { mesh.init(); }
    void deinit() { mesh.deinit(); }
};

struct Renderables 
{
    TwArray<RenderResource, 1024> resources;
    GLProgram fwdProg;
    GLProgram zProg;
    GLProgram defProg;
    GLProgram skyProg;

    DirectionalLight m_light;

    void init();
    void deinit();
    void bindSun(DirectionalLight& light, GLProgram& prog, int channel = TX_SUN_CHANNEL);
    void drawSky(const glm::vec3& eye, const Transform& IVP);
    void shadowPass();
    void prePass(const Transform& VP);
    void fwdDraw(const glm::vec3& eye, const Transform& VP, u32 dflag, s32 width, s32 height);
    void defDraw(const glm::vec3& eye, const Transform& VP, u32 dflag, s32 width, s32 height);
    u16 request();
    void release(u16 handle);
    RenderResource& operator[](u16 i){ return resources[i]; }
};

extern Renderables g_Renderables;
