#pragma once

#include "myglheaders.h"
#include "glm/glm.hpp"
#include "debugmacro.h"
#include "glprogram.h"

#include "store.h"

struct Texture{
    unsigned handle;

    struct parameter{
        const void* ptr;
        int FullType; 
        int Channels; 
        int ComponentType; 
        unsigned width;
        unsigned height;
        bool mip;
    };

    void init(const parameter& p){
        glGenTextures(1, &handle);  MYGLERRORMACRO;
        glBindTexture(GL_TEXTURE_2D, handle);  MYGLERRORMACRO;
        if(p.mip){
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);    MYGLERRORMACRO
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);    MYGLERRORMACRO
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);    MYGLERRORMACRO
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    MYGLERRORMACRO
        }
        else{
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    MYGLERRORMACRO
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    MYGLERRORMACRO
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);    MYGLERRORMACRO
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    MYGLERRORMACRO
        }
        glTexImage2D(GL_TEXTURE_2D, 0, p.FullType, p.width, p.height, 0, p.Channels, p.ComponentType, p.ptr);    MYGLERRORMACRO        
        if(p.mip)
            glGenerateMipmap(GL_TEXTURE_2D);    MYGLERRORMACRO
    }
    void deinit(){
        glDeleteTextures(1, &handle);    MYGLERRORMACRO
    }
    void upload(const parameter& p){
        glTexImage2D(GL_TEXTURE_2D, 0, p.FullType, p.width, p.height, 0, p.Channels, p.ComponentType, p.ptr);  MYGLERRORMACRO;        
        if(p.mip)
            glGenerateMipmap(GL_TEXTURE_2D);    MYGLERRORMACRO
    }
    void bindAlbedo(int channel, GLProgram& prog){
        static const int names[] = { // only works for first prog used
            prog.getUniformLocation("albedoSampler0"),
            prog.getUniformLocation("albedoSampler1"),
            prog.getUniformLocation("albedoSampler2"),
            prog.getUniformLocation("albedoSampler3"),
        };
        glActiveTexture(GL_TEXTURE0 + 2 * channel);  MYGLERRORMACRO;
        glBindTexture(GL_TEXTURE_2D, handle);  MYGLERRORMACRO;
        prog.setUniformInt(names[channel], 2 * channel);
    }
    void bindNormal(int channel, GLProgram& prog){
        static const int names[] = { // only works for first prog used
            prog.getUniformLocation("normalSampler0"),
            prog.getUniformLocation("normalSampler1"),
            prog.getUniformLocation("normalSampler2"),
            prog.getUniformLocation("normalSampler3"),
        };
        glActiveTexture(GL_TEXTURE0 + 2 * channel + 1);  MYGLERRORMACRO;
        glBindTexture(GL_TEXTURE_2D, handle);  MYGLERRORMACRO;
        prog.setUniformInt(names[channel], 2 * channel + 1);
    }
    void setCSBinding(int FullType, int binding){
        glBindImageTexture(binding, handle, 0, GL_FALSE, 0, GL_READ_WRITE, FullType);  MYGLERRORMACRO;
    }

#define TEX_TYPE_MACRO(name, a, b, c) \
    void init##name(unsigned w, unsigned h, bool mip = false, const void* ptr = nullptr){ \
        parameter p = { ptr, a, b, c, w, h, mip }; \
        init(p); \
    }\
    void upload##name(unsigned w, unsigned h, bool mip = false, const void* ptr = nullptr){ \
        parameter p = { ptr, a, b, c, w, h, mip }; \
        upload(p); \
    }

    TEX_TYPE_MACRO(1f, GL_R32F, GL_RED, GL_FLOAT);
    TEX_TYPE_MACRO(2f, GL_RG32F, GL_RG, GL_FLOAT);
    TEX_TYPE_MACRO(4f, GL_RGBA32F, GL_RGBA, GL_FLOAT);

    TEX_TYPE_MACRO(1i, GL_R32I, GL_RED, GL_INT);
    TEX_TYPE_MACRO(2i, GL_RG32I, GL_RG, GL_INT);
    TEX_TYPE_MACRO(4i, GL_RGBA32I, GL_RGBA, GL_INT);

    TEX_TYPE_MACRO(1uc, GL_R8, GL_RED, GL_UNSIGNED_BYTE);
    TEX_TYPE_MACRO(2uc, GL_RG8, GL_RG, GL_UNSIGNED_BYTE);
    TEX_TYPE_MACRO(4uc, GL_RGBA8, GL_RGBA, GL_UNSIGNED_BYTE);
};

struct Image{
    unsigned char* image;
    unsigned width, height;
    bool mip;
};

struct TextureStore{
    Store<Texture, 32> m_store; // kept in gpu memory
    Store<Image, 128> m_images; // kept in cpu memory
    
    void load_texture(Texture& tex, unsigned name, bool need_init);
    Texture* get(unsigned name){
        Texture* m = m_store.get(name);
        if(m){ return m; }

        bool need_init = false;
        if(m_store.full()){
            m = m_store.reuse_near(name);
        }
        else{
            need_init = true;
            m_store.insert(name, {});
            m = m_store[name];
        }

        load_texture(*m, name, need_init);
        return m;
    }
    Texture* operator[](unsigned name){ return get(name); }
    Texture* operator[](const char* name){ return get(hash(name)); }
};

extern TextureStore g_TextureStore;
