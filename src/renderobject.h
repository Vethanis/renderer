#pragma once 

#include "common.h"

#include "mesh.h"
#include "texture.h"
#include "glprogram.h"
#include "camera.h"
#include "transform.h"
#include "UBO.h"
#include "glscreen.h"
#include "depthstate.h"
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
#define DF_GBUFF            10
#define DF_SKY              11
#define DF_VIS_TANGENTS     12
#define DF_VIS_BITANGENTS   13

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

struct RenderResource {
    TextureChannels texture_channels;
    MaterialParams material_params;
    HashString mesh;
    HashString transform;

    bool operator==(const RenderResource& other)const{
        return false;
    }
    void init(){
        transform = g_TransformStore.grow();
    }
    void deinit(){
        g_TransformStore.release(transform.m_hash);
    }
    void bind(GLProgram& prog, UBO& material_ubo){
        texture_channels.bind(prog, 0);
        material_ubo.upload(&material_params, sizeof(MaterialParams));
    }
    void draw(){
        Mesh* m = mesh;
        if(m){
            m->draw();
        }
    }
    void setTransform(const Transform& xform){
        Transform* pXform = transform;
        *pXform = xform;
    }
};

struct Renderables {
    Store<RenderResource, 1024> resources;
    UBO materialparam_ubo;
    GLProgram fwdProg;
    GLProgram zProg;
    GLProgram defProg;
    GLProgram skyProg;

    glm::vec3 sunDirection;
    glm::vec3 sunColor;
    float sunIntensity = 25.0f;

    void init(){
        glEnable(GL_DEPTH_TEST); DebugGL();
        glEnable(GL_CULL_FACE); DebugGL();
        glCullFace(GL_BACK); DebugGL();

        DrawMode::init();

        const char* fwdFilenames[] = {
            "vert.glsl",
            "fwdFrag.glsl"
        };
        const char* zFilenames[] = {
            "zvert.glsl",
            "zfrag.glsl"
        };
        const char* defFilenames[] = {
            "vert.glsl",
            "write_to_gbuff.glsl"
        };

        fwdProg.setup(fwdFilenames, 2);
        zProg.setup(zFilenames, 2);
        defProg.setup(defFilenames, 2);

        skyProg.init(); 
        skyProg.addShader(GLScreen::vertexShader());
        int shader = skyProg.addShader("skyfrag.glsl", GL_FRAGMENT_SHADER);
        skyProg.link();
        skyProg.freeShader(shader);

        sunDirection = glm::normalize(glm::vec3(1.0f, 1.0f, 1.0f));
        sunColor = glm::vec3(1.0f, 0.75f, 0.5f);

        materialparam_ubo.init(nullptr, sizeof(MaterialParams), "materialparams_ubo", &fwdProg.m_id, 1);
    }
    void deinit(){
        fwdProg.deinit();
        zProg.deinit();
        defProg.deinit();
        skyProg.deinit();
        materialparam_ubo.deinit();
    }
    void drawSky(const glm::vec3& eye, const Transform& IVP){
        DrawModeContext ctx(GL_LEQUAL, GL_TRUE, 1);
        skyProg.bind();
        
        skyProg.setUniform("IVP", IVP);
        skyProg.setUniform("sunDirection", sunDirection);
        skyProg.setUniform("eye", eye);
        skyProg.setUniformFloat("sunIntensity", sunIntensity);

        GLScreen::draw();
    }
    void prePass(const Transform& VP){
        DepthContext less(GL_LESS);
        DepthMaskContext dmask(GL_TRUE);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();
        ColorMaskContext nocolor(0);

        zProg.bind();
        for(auto& res : resources){
            Transform* M = res.transform;
            zProg.setUniform("MVP", VP * *M);
            res.draw();
        }
    }
    void fwdDraw(const Camera& cam, const Transform& VP, u32 dflag, s32 width, s32 height){
        glViewport(0, 0, width, height); DebugGL();

        prePass(VP);

        DrawModeContext ctx(GL_LEQUAL, GL_FALSE, 1);

        fwdProg.bind();

        fwdProg.setUniform("sunDirection", sunDirection);
        fwdProg.setUniform("sunColor", sunColor);
        fwdProg.setUniformFloat("sunIntensity", sunIntensity);

        fwdProg.setUniform("eye", cam.getEye());
        fwdProg.setUniformInt("seed", rand());
        fwdProg.setUniformInt("draw_flags", dflag);
        
        for(auto& res : resources){
            Transform* M = res.transform;
            const glm::mat3 IM = glm::inverse(glm::transpose(glm::mat3(*M)));

            fwdProg.setUniform("MVP", VP * *M);
            fwdProg.setUniform("M", *M);
            fwdProg.setUniform("IM", IM);

            res.bind(fwdProg, materialparam_ubo);
            res.draw();
        }

        drawSky(cam.getEye(), glm::inverse(VP));
    }
    void defDraw(const Camera& cam, const Transform& VP, u32 dflag, s32 width, s32 height){
        glViewport(0, 0, width, height); DebugGL();
        
        prePass(VP);

        DrawModeContext ctx(GL_LEQUAL, GL_FALSE, 1);

        defProg.bind();
        defProg.setUniform("eye", cam.getEye());
        defProg.setUniformInt("draw_flags", dflag);

        for(auto& res : resources){
            Transform* M = res.transform;
            const glm::mat3 IM = glm::inverse(glm::transpose(glm::mat3(*M)));

            defProg.setUniform("MVP", VP * *M);
            defProg.setUniform("M", *M);
            defProg.setUniform("IM", IM);

            res.bind(defProg, materialparam_ubo);
            res.draw();
        }
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
        pRes->setTransform(xform);

        return handle;
    }
};

extern Renderables g_Renderables;
