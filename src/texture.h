#pragma once

#include "myglheaders.h"
#include "glm/glm.hpp"
#include "debugmacro.h"
#include "assetstore.h"

struct Texture{
    unsigned handle = 0;

    struct parameter{
        const void* ptr;
        int FullType; 
        int Channels; 
        int ComponentType; 
        int width;
        int height;
        bool mip;
    };

    bool operator==(const Texture& other)const{
        return handle == other.handle;
    }
    void init(const parameter& p){
        glGenTextures(1, &handle);  DebugGL();
        glBindTexture(GL_TEXTURE_2D, handle);  DebugGL();
        if(p.mip){
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);    DebugGL();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);    DebugGL();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);    DebugGL();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    DebugGL();
        }
        else{
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);    DebugGL();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);    DebugGL();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);    DebugGL();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    DebugGL();
        }
        glTexImage2D(GL_TEXTURE_2D, 0, p.FullType, p.width, p.height, 0, p.Channels, p.ComponentType, p.ptr);    DebugGL();        
        if(p.mip)
            glGenerateMipmap(GL_TEXTURE_2D);    DebugGL();
    }
    void deinit(){
        glDeleteTextures(1, &handle);    DebugGL();
    }
    void upload(const parameter& p){
        glBindTexture(GL_TEXTURE_2D, handle);  DebugGL();
        glTexImage2D(GL_TEXTURE_2D, 0, p.FullType, p.width, p.height, 0, p.Channels, p.ComponentType, p.ptr);  DebugGL();
        if(p.mip)
            glGenerateMipmap(GL_TEXTURE_2D);    DebugGL();
    }
    void uploadPortion(int level, int x, int y, int w, int h, int format, int type, const void* p){
        glTextureSubImage2D(handle, level, x, y, w, h, format, type, p);
    }
    void bind(int channel){
        glActiveTexture(GL_TEXTURE0 + channel);  DebugGL();
        glBindTexture(GL_TEXTURE_2D, handle);  DebugGL();
    }
    void setCSBinding(int FullType, int binding){
        glBindImageTexture(binding, handle, 0, GL_FALSE, 0, GL_READ_WRITE, FullType);  DebugGL();
    }

#define TEX_TYPE_MACRO(name, a, b, c) \
    void init##name(int w, int h, bool mip, const void* ptr){ \
        parameter p = { ptr, a, b, c, w, h, mip }; \
        init(p); \
    }\
    void upload##name(int w, int h, bool mip, const void* ptr){ \
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

    TEX_TYPE_MACRO(1us, GL_R16, GL_RED, GL_UNSIGNED_SHORT);
    TEX_TYPE_MACRO(2us, GL_RG16, GL_RG, GL_UNSIGNED_SHORT);
    TEX_TYPE_MACRO(4us, GL_RGBA16, GL_RGBA, GL_UNSIGNED_SHORT);
};

struct Image
{
    unsigned char* image;
    unsigned id;
    int width, height;
    bool mip;
    Image() : image(nullptr), id(0), width(0), height(0), mip(true){}
    bool operator==(const Image& other)const{
        return id == other.id;
    }
    void load(unsigned name);
    void free();
};

struct TextureStoreElement
{
    Texture m_texture;
    int m_refs = 0;
    void AddRef(){ m_refs++;}
    void RemoveRef(){ m_refs--;}
    int RefCount()const{ return m_refs; }
    void OnLoad(unsigned name);
    void OnRelease(unsigned name);
    Texture* GetItem(){ return &m_texture; }
};

struct ImageStoreElement
{
    Image m_image;
    int m_refs = 0;
    void AddRef(){ m_refs++;}
    void RemoveRef(){ m_refs--;}
    int RefCount()const{ return m_refs; }
    void OnLoad(unsigned name);
    void OnRelease(unsigned name){ m_image.free();}
    Image* GetItem(){ return &m_image; }
};

extern AssetStore<TextureStoreElement, Texture, 32> g_TextureStore;
extern AssetStore<ImageStoreElement, Image, 128> g_ImageStore;

inline void TextureStoreElement::OnLoad(unsigned name)
{
    g_ImageStore.request(name);
}

inline void TextureStoreElement::OnRelease(unsigned name)
{ 
    m_texture.deinit();
    g_ImageStore.release(name);
}

inline void ImageStoreElement::OnLoad(unsigned name)
{ 
    m_image.load(name);
    Texture* pTexture = g_TextureStore[name];
    assert(pTexture);
    pTexture->init4uc(m_image.width, m_image.height, m_image.mip, m_image.image);
}