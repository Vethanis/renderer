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
    void addMaterial(const Material& mat){ 
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
    inline glm::mat4& getTransform();
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
        static const int m_name = prog.getUniformLocation("M");
        static const int im_name = prog.getUniformLocation("IM");
        prog.bind();
        for(unsigned i = 0; i < tail; i++){
            glm::mat4& M = transforms[objects[i].transform];
            glm::mat3 IM = glm::inverse(glm::transpose(glm::mat3(M)));
            prog.setUniform(mvp_name, VP * M);
            prog.setUniform(m_name, M);
            prog.setUniform(im_name, IM);
            objects[i].bind(prog);
            g_MeshStore[objects[i].mesh]->draw();
        }
    }
    unsigned grow(){
        assert(tail < capacity);
        objects[tail].transform = tail;
        return tail++;
    }
    void sortByBucket(){
        std::sort(objects, objects + tail);
    }
    RenderResource& operator[](unsigned i){ return objects[i]; }
};

extern Renderables g_Renderables;

inline glm::mat4& RenderResource::getTransform(){
    return g_Renderables.transforms[transform];
}

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