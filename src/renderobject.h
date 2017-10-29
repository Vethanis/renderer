#pragma once 

#include "common.h"
#include "hashstring.h"
#include "glprogram.h"
#include "transform.h"
#include "UBO.h"
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

#define ODF_DEFAULT         0
#define ODF_SKY             1

#define TX_ALBEDO_CHANNEL   0
#define TX_MATERIAL_CHANNEL 1
#define TX_SUN_CHANNEL      2
#define TX_CUBEMAP_CHANNEL  3
#define TX_POSITION_CHANNEL 4
#define TX_NORMAL_CHANNEL   5

// ------------------------------------------------------------------------

struct TextureChannels 
{
    HashString albedo;
    HashString material;

    void bind(GLProgram& prog, int channel);
};

struct MaterialParams 
{
    float roughness_offset = 0.0f;
    float roughness_multiplier = 1.0f;
    float metalness_offset = 0.0f;
    float metalness_multiplier = 1.0f;
    float index_of_refraction = 1.0f;
    float bumpiness = 1.0f;
    float heightScale = 0.0075f;
    float _pad3;
};

struct RenderResource 
{
    TextureChannels texture_channels;
    MaterialParams material_params;
    HashString mesh;
    HashString transform;

    bool operator==(const RenderResource& other)const{ return false; };
    void init();
    void deinit();
    void bind(GLProgram& prog, UBO& material_ubo);
    void draw();
    void setTransform(const Transform& xform);
};

struct Renderables 
{
    Store<RenderResource, 1024> resources;
    UBO materialparam_ubo;
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
    HashString grow();
    void release(HashString handle);
    RenderResource* operator[](unsigned i);
    HashString create(HashString mesh, HashString albedo, 
        HashString material, const Transform& xform = Transform(), 
        float roughness = 0.0f, float metalness = 0.0f, 
        unsigned flags = 0);
};

extern Renderables g_Renderables;
