#pragma once 

#include "mesh.h"
#include "texture.h"
#include "glprogram.h"
#include "glscreen.h"
#include "SSBO.h"
#include "light.h"
#include "camera.h"
#include "gpu_octree.h"
#include "array.h"
#include "transform.h"
#include "hashstring.h"

struct Material{
    HashString albedo;
    HashString normal;

    void bind(GLProgram& prog, int channel){
        {
            Texture* ta = albedo;
            ta->bindAlbedo(channel);
            prog.bindAlbedo(channel);
        }
        {
            Texture* tn = normal;
            tn->bindNormal(channel);
            prog.bindNormal(channel);
        }
    }
};

struct RenderResource{
    Array<Material, 4> materials;
    HashString mesh;
    HashString transform;

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
};

struct Renderables{
    Array<RenderResource, 256> resources;
    GLProgram prog;
    oct::gpu_octree tree;
    void init();
    void deinit(){
        prog.deinit();
        tree.deinit();
    }
    void draw(const Transform& VP){
        static const int mvp_name = prog.getUniformLocation("MVP");
        static const int m_name = prog.getUniformLocation("M");
        static const int im_name = prog.getUniformLocation("IM");
        prog.bind();
        tree.upload();
        for(RenderResource& res : resources){
            Transform* M = res.transform;
            glm::mat3 IM = glm::inverse(glm::transpose(glm::mat3(*M)));
            prog.setUniform(mvp_name, VP * *M);
            prog.setUniform(m_name, *M);
            prog.setUniform(im_name, IM);
            res.bind(prog);
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
};

extern Renderables g_Renderables;

struct GBuffer{
    unsigned buff;
    unsigned rboDepth;
    unsigned posbuff, normbuff, matbuff;
    unsigned width, height;
    GLProgram prog;
    SSBO lightbuff;
    void init(int w, int h);
    void deinit();
    void updateLights(const LightSet& lights);
    void draw(const Camera& cam);
};

extern GBuffer g_gBuffer;