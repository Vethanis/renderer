#pragma once

#include "myglheaders.h"
#include "glm/glm.hpp"
#include "debugmacro.h"
#include "store.h"
#include "circular_queue.h"
#include <thread>
#include <mutex>
#include <chrono>

struct Texture{
    unsigned handle;

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
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);    DebugGL();
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);    DebugGL();
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

struct Image{
    unsigned char* image;
    int width, height;
    bool mip;
    Image() : image(nullptr), width(0), height(0), mip(true){}
    bool operator==(const Image& other)const{
        return image == other.image;
    }
};

struct ImageStore{
    Store<Image, 256> m_store;
    CircularQueue<unsigned, 256> m_queue;
    std::thread m_thread;
    std::mutex m_mutex;
    bool m_shouldRun;

    void processQueue(){
        while(m_shouldRun){

            if(!m_queue.empty() && m_mutex.try_lock())
            {
                while(m_shouldRun && !m_queue.empty()){
                    unsigned name = m_queue.pop();
                    Image* m = nullptr;
                    if(m_store.full()){
                        m = m_store.reuse_near(name);
                    }
                    else{
                        m_store.insert(name, Image());
                        m = m_store[name];
                    }
            
                    load_image(*m, name);
                }
                m_mutex.unlock();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }

    ImageStore(){
        m_shouldRun = true;
        m_thread = std::thread(&ImageStore::processQueue, this);
    }
    ~ImageStore(){
        m_shouldRun = false;
        m_thread.join();
    }

    void load_image(Image& img, unsigned name);
    Image* get(unsigned name){
        Image* m = m_store[name];
        if(m){return m;}

        if(m_queue.full() == false){
            m_queue.set_push(name);
        }
        return nullptr;
    }
    Image* operator[](unsigned name){ return get(name); }
};

extern ImageStore g_ImageStore;

struct TextureStore{
    Store<Texture, 32> m_store;

    Texture* get(unsigned name){
        Texture* m = m_store[name];
        if(m){return m;}

        if(g_ImageStore.m_mutex.try_lock())
        {
            Image* img = g_ImageStore[name];
            if(img){
                if(m_store.full()){
                    m = m_store.reuse_near(name);
                    m->upload4uc(img->width, img->height, img->mip, img->image);
                }
                else{
                    m_store.insert(name, {});
                    m = m_store[name];
                    m->init4uc(img->width, img->height, img->mip, img->image);
                }
            }
            g_ImageStore.m_mutex.unlock();
        }

        return m;
    }
    Texture* operator[](unsigned name){
        return get(name);
    }
};

extern TextureStore g_TextureStore;
