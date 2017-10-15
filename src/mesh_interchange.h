#pragma once

#include "common.h"
#include "vertexbuffer.h"

struct aiMesh;
struct aiScene;
struct aiNode;

namespace mesh_interchange 
{
    struct Model
    {        
        VertexBuffer vertices;
        IndexBuffer indices;
        void serialize(FILE* pFile)
        {
            vertices.serialize(pFile);
            indices.serialize(pFile);
        }
        void load(FILE* pFile)
        {
            vertices.load(pFile);
            indices.load(pFile);
        }
        u32 hash()const 
        {
            u32 val = 0;
            val ^= vertices.hash();
            val ^= indices.hash();
            return val;
        }
    
        void processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& xform);
        void processNode(aiNode* node, const aiScene* scene, const glm::mat4& xform);
        void parse(const char* filename);
        bool operator==(const Model& other)const
        {
            return hash() == other.hash();
        }
        bool operator==(const Model& other)
        {
            return hash() == other.hash();
        }
        void clear()
        {
            vertices.clear();
            indices.clear();
        }
    };
};
