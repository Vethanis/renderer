#pragma once 

#include "common.h"

#include "mesh.h"
#include "texture.h"
#include "glprogram.h"
#include "glscreen.h"
#include "light.h"
#include "camera.h"
#include "transform.h"
#include "cubemap.h"
#include "UBO.h"
#include <random>

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

#define ODF_DEFAULT         0
#define ODF_SKY             1

// ------------------------------------------------------------------------

struct TextureChannels {
    HashString albedo;
    HashString material;

    void bind(GLProgram& prog, int channel){
        Texture* t = albedo;
        if(t){
            t->bind(0);
            prog.setUniformInt("albedoSampler", 0);
        }
        t = material;
        if(t){
            t->bind(1);
            prog.setUniformInt("materialSampler", 1);
        }
    }
};

struct MaterialParams {
    float roughness_offset = 0.0f;
    float roughness_multiplier = 1.0f;
    float metalness_offset = 0.0f;
    float metalness_multiplier = 1.0f;
    float index_of_refraction = 1.0f;
    float bumpiness = 1.0f;
    float _pad2;
    float _pad3;
};

struct RenderResource{
    TextureChannels texture_channels;
    MaterialParams material_params;
    HashString mesh;
    HashString transform;
    u32 object_flag;

    bool operator==(const RenderResource& other)const{
        return false;
    }
    void init(){
        transform = g_TransformStore.grow();
        object_flag = 0;
    }
    void deinit(){
        g_TransformStore.release(transform.m_hash);
    }
    void bind(GLProgram& prog, UBO& material_ubo){
        prog.setUniformInt("object_flags", object_flag);
        texture_channels.bind(prog, 0);
        material_ubo.upload(&material_params, sizeof(MaterialParams));
    }
    void draw(){
        Mesh* m = mesh;
        if(m){
            m->draw();
        }
    }
    u32 get_flag(u32 flag) const {
        return object_flag & flag;
    }
    void set_flag(u32 flag){
        object_flag |= flag;
    }
    void unset_flag(u32 flag){
        object_flag &= ~flag;
    }
    void setTransform(const Transform& xform){
        Transform* pXform = transform;
        *pXform = xform;
    }
};

struct Renderables{
    Store<RenderResource, 1024> resources;
    UBO materialparam_ubo;
    GLProgram fwdProg;
    GLProgram zProg;
    Cubemap cm;

    glm::vec3 sunDirection;
    glm::vec3 sunColor;

    void init(){
        glEnable(GL_DEPTH_TEST); DebugGL();
        glEnable(GL_CULL_FACE); DebugGL();
        glCullFace(GL_BACK); DebugGL();

        const char* fwdFilenames[] = {
            "vert.glsl",
            "fwdFrag.glsl"
        };
        const char* zFilenames[] = {
            "zvert.glsl",
            "zfrag.glsl"
        };

        fwdProg.setup(fwdFilenames, 2);
        zProg.setup(zFilenames, 2);

        sunDirection = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
        sunColor = glm::vec3(1.0f, 0.75f, 0.5f);

        cm.init(1024);

        materialparam_ubo.init(nullptr, sizeof(MaterialParams), "materialparams_ubo", &fwdProg.m_id, 1);
    }
    void deinit(){
        fwdProg.deinit();
        zProg.deinit();
        cm.deinit();
        materialparam_ubo.deinit();
    }
    void prePass(const Transform& VP){
        glDepthFunc(GL_LESS); DebugGL();
        glDepthMask(GL_TRUE); DebugGL();
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();
        glColorMask(0,0,0,0); DebugGL();

        zProg.bind();
        for(auto& res : resources){
            Transform* M = res.transform;
            zProg.setUniform("MVP", VP * *M);
            res.draw();
        }

        glDepthFunc(GL_LEQUAL); DebugGL();
        glColorMask(1,1,1,1); DebugGL();
        glDepthMask(GL_FALSE); DebugGL();
    }
    void fwdDraw(const Camera& cam, const Transform& VP, u32 dflag, s32 width, s32 height){
        glViewport(0, 0, width, height);

        #if PREPASS_ENABLED
            prePass(VP);
        #else
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();
        #endif

        fwdProg.bind();

        fwdProg.setUniform("sunDirection", sunDirection);
        fwdProg.setUniform("sunColor", sunColor);

        fwdProg.setUniform("eye", cam.getEye());
        fwdProg.setUniformInt("seed", rand());
        fwdProg.setUniformInt("draw_flags", dflag);
        
        for(auto& res : resources){
            Transform* M = res.transform;
            glm::mat3 IM = glm::inverse(glm::transpose(glm::mat3(*M)));

            fwdProg.setUniform("MVP", VP * *M);
            fwdProg.setUniform("M", *M);
            fwdProg.setUniform("IM", IM);

            res.bind(fwdProg, materialparam_ubo);
            res.draw();
        }
    }
    void mainDraw(const Camera& cam, u32 dflag, s32 width, s32 height){
        cm.drawInto(cam);
        cm.bind(20, fwdProg);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); DebugGL();
        fwdDraw(cam, cam.getVP(), dflag, width, height);
    }
    HashString grow(){
        HashString handle = resources.grow();
        resources[handle.m_hash]->init();
        return handle;
    }
    void release(HashString handle){
        resources.remove(handle.m_hash)->deinit();
    }
    RenderResource* operator[](unsigned i){
        return resources[i];
    }
    HashString create(HashString mesh, HashString albedo, 
        HashString material, const Transform& xform = Transform(), 
        float roughness = 0.0f, float metalness = 0.0f, 
        unsigned flags = 0){

        HashString handle = resources.grow();
        RenderResource* pRes = resources[handle.m_hash];
        pRes->init();
        pRes->mesh = mesh;
        pRes->texture_channels.albedo = albedo;
        pRes->texture_channels.material = material;
        pRes->material_params.roughness_offset = roughness;
        pRes->material_params.metalness_offset = metalness;
        pRes->object_flag = flags;
        pRes->setTransform(xform);

        return handle;
    }
};

extern Renderables g_Renderables;
