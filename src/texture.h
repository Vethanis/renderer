#pragma once

#include "myglheaders.h"
#include "glm/glm.hpp"
#include "debugmacro.h"
#include "glprogram.h"

#include "store.h"

struct Texture{
    int width, height;
    unsigned handle;
    int FullType, Channels, ComponentType;
    void init(int _FullType, int _Channels, int _ComponentType, int w, int h, bool mip){
        FullType = _FullType;
        Channels = _Channels;
        ComponentType = _ComponentType;
        width = w;
        height = h;
        glGenTextures(1, &handle);
        glBindTexture(GL_TEXTURE_2D, handle);
        if(mip){
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
        glTexImage2D(GL_TEXTURE_2D, 0, FullType, width, height, 0, Channels, ComponentType, NULL);    MYGLERRORMACRO
        if(mip)
            glGenerateMipmap(GL_TEXTURE_2D);    MYGLERRORMACRO

        glBindTexture(GL_TEXTURE_2D, 0);
        MYGLERRORMACRO
    }
    void deinit(){
        glDeleteTextures(1, &handle);    MYGLERRORMACRO
    }
    void upload(const void* ptr){
        glTexImage2D(GL_TEXTURE_2D, 0, FullType, width, height, 0, Channels, ComponentType, ptr);
    }
    void bind(int channel, const char* uname, GLProgram& prog){
        glActiveTexture(GL_TEXTURE0 + channel);
        MYGLERRORMACRO
        glBindTexture(GL_TEXTURE_2D, handle);
        MYGLERRORMACRO
        prog.setUniformInt(uname, channel);
    }
    void setCSBinding(int binding){
        glBindImageTexture(0, handle, 0, GL_FALSE, 0, GL_READ_WRITE, FullType);
        MYGLERRORMACRO
    }
    void setPixel(glm::ivec2 cr, const void* p){
        glTextureSubImage2D(handle, 0, cr.x, cr.y, 1, 1, Channels, ComponentType, p);
        MYGLERRORMACRO
    }
#define TEX_INIT_MACRO(name, a, b, c) \
    void init##name(int w, int h, bool mip=false){ \
        init(a, b, c, w, h, mip); \
    }

    TEX_INIT_MACRO(1f, GL_R32F, GL_RED, GL_FLOAT);
    TEX_INIT_MACRO(2f, GL_RG32F, GL_RG, GL_FLOAT);
    TEX_INIT_MACRO(3f, GL_RGB32F, GL_RGB, GL_FLOAT);
    TEX_INIT_MACRO(4f, GL_RGBA32F, GL_RGBA, GL_FLOAT);

    TEX_INIT_MACRO(1i, GL_R32I, GL_RED, GL_INT);
    TEX_INIT_MACRO(2i, GL_RG32I, GL_RG, GL_INT);
    TEX_INIT_MACRO(3i, GL_RGB32I, GL_RGB, GL_INT);
    TEX_INIT_MACRO(4i, GL_RGBA32I, GL_RGBA, GL_INT);

    TEX_INIT_MACRO(1u, GL_R32UI, GL_RED, GL_UNSIGNED_INT);
    TEX_INIT_MACRO(2u, GL_RG32UI, GL_RG, GL_UNSIGNED_INT);
    TEX_INIT_MACRO(3u, GL_RGB32UI, GL_RGB, GL_UNSIGNED_INT);
    TEX_INIT_MACRO(4u, GL_RGBA32UI, GL_RGBA, GL_UNSIGNED_INT);

    TEX_INIT_MACRO(1c, GL_R8I, GL_RED, GL_BYTE);
    TEX_INIT_MACRO(2c, GL_RG8I, GL_RG, GL_BYTE);
    TEX_INIT_MACRO(3c, GL_RGB8I, GL_RGB, GL_BYTE);
    TEX_INIT_MACRO(4c, GL_RGBA8I, GL_RGBA, GL_BYTE);

    TEX_INIT_MACRO(1uc, GL_R8UI, GL_RED, GL_UNSIGNED_BYTE);
    TEX_INIT_MACRO(2uc, GL_RG8UI, GL_RG, GL_UNSIGNED_BYTE);
    TEX_INIT_MACRO(3uc, GL_RGB8UI, GL_RGB, GL_UNSIGNED_BYTE);
    TEX_INIT_MACRO(4uc, GL_RGBA8UI, GL_RGBA, GL_UNSIGNED_BYTE);
};

struct TextureStore{
    Store<Texture, 32> m_store;
    
    void load_texture(Texture& tex, unsigned name);
    Texture* get(unsigned name){
        Texture* m = m_store.get(name);
        if(m){ return m; }

        if(m_store.full()){
            m_store.remove_near(name)->deinit();
        }

        Texture nm;
        load_texture(nm, name);
        m_store.insert(name, nm);
        m = m_store.get(name);
        assert(m);
        return m;
    }
    Texture* operator[](unsigned name){ return get(name); }
    Texture* operator[](const char* name){ return get(hash(name)); }
};

extern TextureStore g_TextureStore;
