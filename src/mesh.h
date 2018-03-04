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
    void OnLoad(unsigned name);
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
    void OnLoad(unsigned name);
    void OnRelease(unsigned name);
};

extern AssetStore<GeometryStoreElement, Geometry, 2048> g_GeometryStore;
extern AssetStore<MeshStoreElement, Mesh, 512> g_MeshStore;

inline void GeometryStoreElement::OnLoad(unsigned name)
{
    WorldTile tile(name);
    CreateMesh(m_geometry, tile.m_sdfs, tile.m_sdfDepth);
    Mesh* pMesh = g_MeshStore[name];
    pMesh->upload(m_geometry);
};

inline void GeometryStoreElement::OnRelease(unsigned name)
{

}

inline void MeshStoreElement::OnLoad(unsigned name)
{ 
    m_mesh.init();
    g_GeometryStore.request(name);
};

inline void MeshStoreElement::OnRelease(unsigned name)
{ 
    m_mesh.deinit();
    g_GeometryStore.release(name);
}