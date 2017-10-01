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
        t->bind(0);
        prog.setUniformInt("albedoSampler", 0);
        t = material;
        t->bind(1);
        prog.setUniformInt("materialSampler", 1);
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
    u32 handle;

    RenderResource(){
        static u32 s_handle = 0;
        object_flag = 0;
        handle = s_handle++;
    }
    void bind(GLProgram& prog, UBO& material_ubo){
        prog.setUniformInt("object_flags", object_flag);
        texture_channels.bind(prog, 0);
        material_ubo.upload(&material_params, sizeof(MaterialParams));
    }
    unsigned bucket() const {
        const u32 thash = fnv(&texture_channels, sizeof(TextureChannels)) << 16;
        return thash | (0xffff & mesh);
    }
    bool operator < (const RenderResource& o) const {
        return bucket() < o.bucket();
    }
    bool operator > (const RenderResource& o) const {
        return bucket() > o.bucket();
    }
    void draw(){
        Mesh* m = mesh;
        m->draw();
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
};

struct Renderables{
    Array<RenderResource, 256> resources;
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
        for(RenderResource& res : resources){
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
        
        for(RenderResource& res : resources){
            Transform* M = res.transform;
            glm::mat3 IM = glm::inverse(glm::transpose(glm::mat3(*M)));

            fwdProg.setUniform("MVP", VP * *M);
            fwdProg.setUniform("M", *M);
            fwdProg.setUniform("IM", IM);

            res.bind(fwdProg, materialparam_ubo);
            res.draw();
        }
    }
    RenderResource& grow(){
        RenderResource& r = resources.grow();
        r.transform = g_TransformStore.grow();
        return r;
    }
    RenderResource* find(u32 handle){
        for(auto& i : resources){
            if(i.handle == handle){
                return &i;
            }
        }
        return nullptr;
    }
    void finishGrow(){
        resources.sort();
    }
    void mainDraw(const Camera& cam, u32 dflag, s32 width, s32 height){
        cm.drawInto(cam);
        cm.bind(20, fwdProg);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); DebugGL();
        fwdDraw(cam, cam.getVP(), dflag, width, height);
    }
};

extern Renderables g_Renderables;
