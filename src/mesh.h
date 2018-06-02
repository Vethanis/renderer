#pragma once

#include "ints.h"
#include "assetstore.h"
#include "vertexbuffer.h"
#include "meshgen.h"
#include "worldgen.h"

struct Mesh 
{
    u32 vao, vbo, ebo, num_indices;
    void draw();
    void upload(const Geometry& geom);
    void init();
    void deinit();
};

struct GeometryStoreElement
{
    Geometry m_item;
    s32 m_refs = 0;
};

struct MeshStoreElement
{
    Mesh m_item;
    s32 m_refs = 0;
};

extern AssetStore<GeometryStoreElement, Geometry, 2048> g_GeometryStore;
extern AssetStore<MeshStoreElement, Mesh, 512> g_MeshStore;

template<>
inline void OnLoadAsync(GeometryStoreElement* item, unsigned name)
{
    SDFDefinition* pDef = g_SdfStore[name];
    assert(pDef);
    CreateMesh(item->m_item, pDef->m_sdfs, pDef->m_sdfDepth);
    if(pDef->m_deleteOnUse)
    {
        g_SdfStore.remove(name);
    }
};

template<>
inline void OnLoadSync(GeometryStoreElement* item, u32 name)
{
    Mesh* pMesh = g_MeshStore[name];
    if(pMesh)
    {
        pMesh->upload(item->m_item);
    }
}

template<>
inline void OnLoadSync(MeshStoreElement* item, u32 name)
{ 
    item->m_item.init();
    g_GeometryStore.request(name);
};

template<>
inline void OnRelease(MeshStoreElement* item, u32 name)
{ 
    item->m_item.deinit();
    g_GeometryStore.release(name);
}