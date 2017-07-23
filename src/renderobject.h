#pragma once 

#include "glm/glm.hpp"
#include "mesh.h"
#include "texture.h"
#include "glprogram.h"
#include "glscreen.h"
#include "SSBO.h"
#include "light.h"

struct RenderObject{
    glm::mat4 transform;
    unsigned albedo;
    unsigned specular;
    unsigned mesh;
    bool valid(){
        return g_TextureStore.get(albedo) &&
            g_TextureStore.get(specular) &&
            g_MeshStore.get(mesh);
    }
    void draw(const glm::mat4& VP, GLProgram& prog){
        prog.setUniform("MVP", VP * transform);
        g_TextureStore[albedo]->bind(0, "albedoSampler", prog);
        g_TextureStore[specular]->bind(1, "specularSampler", prog);
        g_MeshStore[mesh]->draw();
    }
};

struct Renderables{
    static constexpr unsigned capacity = 1024;
    RenderObject objects[capacity];
    unsigned tail;
    GLProgram prog;
    void init();
    void deinit(){ prog.deinit(); }
    void draw(const glm::mat4& VP){
        for(unsigned i = 0; i < tail; i++){
            objects[i].draw(VP, prog);
        }
    }
    unsigned add(const RenderObject& obj){
        assert(tail < capacity);
        objects[tail] = obj;
        if(objects[tail].valid()){
            return tail++;
        }
        return unsigned(-1);
    }
};

extern Renderables g_Renderables;

struct GBuffer{
    unsigned buff;
    unsigned posbuff, normbuff, matbuff;
    GLScreen screen;
    GLProgram prog;
    SSBO lightbuff;
    void init(int w, int h);
    void deinit();
    void updateLights(const LightSet& lights);
    void draw(const glm::vec3& eye);
};

extern GBuffer g_gBuffer;