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
#include <random>

// ------------------------------------------------------------------------

#define PREPASS_ENABLED 1

#define DF_DIRECT       0
#define DF_INDIRECT     1
#define DF_NORMALS      2
#define DF_REFLECT      3
#define DF_UV           4
#define DF_VIS_CUBEMAP  6
#define DF_VIS_REFRACT  7

#define ODF_DEFAULT     0
#define ODF_SKY         1

// ------------------------------------------------------------------------

struct Material{
    HashString albedo;
    HashString normal;
    void bind(GLProgram& prog, int channel){
        Texture* ta = albedo;
        ta->bindAlbedo(channel);
        prog.bindAlbedo(channel);
        Texture* tn = normal;
        tn->bindNormal(channel);
        prog.bindNormal(channel);
    }
};

struct RenderResource{
    Array<Material, 4> materials;
    HashString mesh;
    HashString transform;
    u32 oflag = 0;

    void bind(GLProgram& prog){
        for(int i = 0; i < materials.count(); ++i){
            materials[i].bind(prog, i);
        }
    }
    void addMaterial(const Material& mat){
        materials.grow() = mat;
    }
    unsigned bucket()const{
        return materials.hash() << 16 | (0xffff & mesh);
    }
    bool operator < (const RenderResource& o)const{
        return bucket() < o.bucket();
    }
    bool operator > (const RenderResource& o)const{
        return bucket() > o.bucket();
    }
    void draw(){
        Mesh* m = mesh;
        m->draw();
    }
    u32 get_flag(u32 flag)const{
        return oflag & flag;
    }
    void set_flag(u32 flag){
        oflag |= flag;
    }
    void unset_flag(u32 flag){
        oflag &= ~flag;
    }
};

struct Renderables{
    Array<RenderResource, 256> resources;
    GLProgram fwdProg;
    GLProgram zProg;
    Cubemap cm;
    glm::vec3 sunDirection;
    glm::vec3 sunColor;
    float iorr;
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
        iorr = 0.9f;

        cm.init(2048);
    }
    void deinit(){
        fwdProg.deinit();
        zProg.deinit();
        cm.deinit();
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
    void fwdDraw(const Camera& cam, const Transform& VP, u32 dflag){
        #if PREPASS_ENABLED
            prePass(VP);
        #else
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); DebugGL();
        #endif

        fwdProg.bind();
        fwdProg.setUniform("sunDirection", sunDirection);
        fwdProg.setUniform("sunColor", sunColor);
        fwdProg.setUniformInt("seed", rand());
        fwdProg.setUniform("eye", cam.getEye());
        fwdProg.setUniformInt("draw_flags", dflag);
        fwdProg.setUniformFloat("iorr", iorr);
        
        for(RenderResource& res : resources){

            Transform* M = res.transform;
            glm::mat3 IM = glm::inverse(glm::transpose(glm::mat3(*M)));

            fwdProg.setUniform("MVP", VP * *M);
            fwdProg.setUniform("M", *M);
            fwdProg.setUniform("IM", IM);
            fwdProg.setUniformInt("object_flags", res.oflag);

            res.bind(fwdProg);
            res.draw();
        }
    }
    RenderResource& grow(){
        RenderResource& r = resources.grow();
        r.transform = g_TransformStore.grow();
        return r;
    }
    void finishGrow(){
        resources.sort();
    }
    void mainDraw(const Camera& cam, u32 dflag){
        cm.drawInto(cam);
        cm.bind(20, fwdProg);
        glBindFramebuffer(GL_FRAMEBUFFER, 0); DebugGL();
        fwdDraw(cam, cam.getVP(), dflag);
    }
};

extern Renderables g_Renderables;
