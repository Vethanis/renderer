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
    Geometry m_geometry;
    int m_refs = 0;
    void AddRef(){ m_refs++; }
    void RemoveRef(){ m_refs--; }
    int RefCount() const { return m_refs; }
    Geometry* GetItem(){ return &m_geometry; }
    void OnLoadSync(unsigned name);
    void OnLoadAsync(unsigned name);
    void OnRelease(unsigned name);
};

struct MeshStoreElement
{
    Mesh m_mesh;
    int m_refs = 0;
    void AddRef(){ m_refs++; }
    void RemoveRef(){ m_refs--; }
    int RefCount() const { return m_refs; }
    Mesh* GetItem(){ return &m_mesh; }
    void OnLoadSync(unsigned name);
    void OnLoadAsync(unsigned){}
    void OnRelease(unsigned name);
};

extern AssetStore<GeometryStoreElement, Geometry, 2048> g_GeometryStore;
extern AssetStore<MeshStoreElement, Mesh, 512> g_MeshStore;

inline void GeometryStoreElement::OnLoadAsync(unsigned name)
{
    SDFDefinition* pDef = g_SdfStore[name];
    assert(pDef);
    CreateMesh(m_geometry, pDef->m_sdfs, pDef->m_sdfDepth);
    if(pDef->m_deleteOnUse)
    {
        g_SdfStore.remove(name);
    }
};

inline void GeometryStoreElement::OnLoadSync(unsigned name)
{
    Mesh* pMesh = g_MeshStore[name];
    if(pMesh)
    {
        pMesh->upload(m_geometry);
    }
}

inline void GeometryStoreElement::OnRelease(unsigned name)
{

}

inline void MeshStoreElement::OnLoadSync(unsigned name)
{ 
    m_mesh.init();
    g_GeometryStore.request(name);
};

inline void MeshStoreElement::OnRelease(unsigned name)
{ 
    m_mesh.deinit();
    g_GeometryStore.release(name);
}