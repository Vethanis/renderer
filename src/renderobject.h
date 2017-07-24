#pragma once 

#include "glm/glm.hpp"
#include "mesh.h"
#include "texture.h"
#include "glprogram.h"
#include "glscreen.h"
#include "SSBO.h"
#include "light.h"
#include "camera.h"

struct Material{
    static constexpr unsigned num_textures = 2;
    unsigned albedo;
    unsigned normal;
};

struct RenderResource{
    static constexpr unsigned max_channels = 4;
    Material channels[max_channels];
    unsigned num_channels = 0;
    unsigned mesh;
    void bind(GLProgram& prog){
        for(unsigned i = 0; i < num_channels; i++){
            g_TextureStore[channels[i].albedo]->bindAlbedo(i, prog);
            g_TextureStore[channels[i].normal]->bindNormal(i, prog);
        }
    }
    bool valid(){
        bool is_valid = true;
        for(unsigned i = 0; i < num_channels; i++){
            is_valid = is_valid && g_TextureStore[channels[i].albedo];
            is_valid = is_valid && g_TextureStore[channels[i].normal];
        }
        return is_valid;
    }
    void add(const Material& mat){ 
        assert(num_channels < max_channels); 
        channels[num_channels++] = mat;
    }
};

struct RenderObject{
    glm::mat4 transform;
    RenderResource resource;
    bool valid(){ return resource.valid(); }
    void draw(const glm::mat4& VP, GLProgram& prog){
        static const unsigned mvp_name = hash("MVP");
        prog.setUniform(mvp_name, VP * transform);
        resource.bind(prog);
        g_MeshStore[resource.mesh]->draw();
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
        prog.bind();
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
    RenderObject& operator[](unsigned i){ return objects[i]; }
};

extern Renderables g_Renderables;

struct GBuffer{
    unsigned buff;
    unsigned rboDepth;
    unsigned posbuff, normbuff, matbuff;
    unsigned width, height;
    GLScreen screen;
    GLProgram prog;
    SSBO lightbuff;
    void init(int w, int h);
    void deinit();
    void updateLights(const LightSet& lights);
    void draw(const Camera& cam);
};

extern GBuffer g_gBuffer;