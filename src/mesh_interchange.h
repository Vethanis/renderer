#pragma once

#include "common.h"
#include "vertexbuffer.h"

struct aiMesh;
struct aiScene;
struct aiNode;

namespace mesh_interchange {
    static constexpr s32 MAX_MATERIALS_PER_MESH = 4;
    struct Material{
        Vector<HashString> albedo;
        Vector<HashString> normal;
    
        u32 id(){
            return albedo.hash() ^ normal.hash();
        }
        void serialize(FILE* pFile){
            albedo.serialize_composite(pFile);
            normal.serialize_composite(pFile);
        }
        void load(FILE* pFile){
            albedo.load_composite(pFile);
            normal.load_composite(pFile);
        }
        u32 hash()const {
            u32 val = 0;
            val ^= albedo.hash();
            val ^= normal.hash();
            return val;
        }
    };
    struct Geometry{
        VertexBuffer vertices;
        IndexBuffer indices;
        void serialize(FILE* pFile){
            vertices.serialize(pFile);
            indices.serialize(pFile);
        }
        void load(FILE* pFile){
            vertices.load(pFile);
            indices.load(pFile);
        }
        u32 hash()const {
            u32 val = 0;
            val ^= vertices.hash();
            val ^= indices.hash();
            return val;
        }
    };
    struct Model{
        Geometry meshes;
        Vector<Material> materials;
    
        void processMesh(aiMesh* mesh, const aiScene* scene,
            const glm::mat4& xform, 
            Geometry& out, Vector<HashString>& mat_ids);
        void processNode(aiNode* node, const aiScene* scene, 
            const glm::mat4& xform,
            Vector<HashString>& mat_ids);
        void parse(const char* filename);
        bool operator==(const Model& other){
            return hash() == other.hash();
        }
        void serialize(FILE* pFile){
            meshes.serialize(pFile);
            materials.serialize_composite(pFile);
        }
        void load(FILE* pFile){
            meshes.load(pFile);
            materials.load_composite(pFile);
        }
        u32 hash()const {
            u32 val = 0;
            val ^= meshes.hash();
            for(const auto& mat : materials){
                val ^= mat.hash();
            }
            return val;
        }
    };
};
