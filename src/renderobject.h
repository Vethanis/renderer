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
    unsigned albedo;
    unsigned normal;
};

struct RenderResource{
    static constexpr unsigned max_channels = 4;
    Material channels[max_channels];
    unsigned num_channels;
    unsigned mesh;
    unsigned transform;
    RenderResource(){
        memset(this, 0, sizeof(*this));
    }
    void bind(GLProgram& prog){
        for(unsigned i = 0; i < num_channels; i++){
            g_TextureStore[channels[i].albedo]->bindAlbedo(i, prog);
            g_TextureStore[channels[i].normal]->bindNormal(i, prog);
        }
    }
    bool valid()const{
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
    bool operator<(const RenderResource& o)const{
        unsigned matHash = hash(channels, sizeof(Material) * num_channels);
        unsigned bucket = matHash << 16 | mesh;

        unsigned oHash = hash(o.channels, sizeof(Material) * o.num_channels);
        unsigned oBucket = oHash << 16 | o.mesh;

        return bucket < oBucket;
    }
};

struct Renderables{
    static constexpr unsigned capacity = 1024;
    glm::mat4 transforms[capacity];
    RenderResource objects[capacity];
    unsigned tail;
    GLProgram prog;
    void init();
    void deinit(){ prog.deinit(); }
    void draw(const glm::mat4& VP){
        static const int mvp_name = prog.getUniformLocation("MVP");
        prog.bind();
        for(unsigned i = 0; i < tail; i++){
            prog.setUniform(mvp_name, VP * transforms[objects[i].transform]);
            objects[i].bind(prog);
            g_MeshStore[objects[i].mesh]->draw();
        }
    }
    unsigned add(const RenderResource& obj){
        assert(tail < capacity);
        objects[tail] = obj;
        objects[tail].transform = tail;
        if(objects[tail].valid()){
            ++tail;
            std::sort(objects, objects + tail);
            return tail - 1;
        }
        return unsigned(-1);
    }
    RenderResource& operator[](unsigned i){ return objects[i]; }
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