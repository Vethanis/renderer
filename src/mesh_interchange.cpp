
#define GLM_SWIZZLE
#include "mesh_interchange.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mesh_interchange
{
    glm::mat4 getTransform(aiNode* node)
    {
        glm::mat4 SRT;
        aiVector3D scale, translation;
        aiQuaternion rotation;
        node->mTransformation.Decompose(scale, rotation, translation);
        glm::mat4 S = glm::scale({}, glm::vec3(scale.x, scale.y, scale.z));
        glm::quat rot;
        rot.w = rotation.w;
        rot.x = rotation.x;
        rot.y = rotation.y;
        rot.z = rotation.z;
        glm::mat4 R = glm::mat4_cast(rot);
        glm::mat4 T = glm::translate({}, 
            glm::vec3(translation.x, translation.y, translation.z));
        SRT = T * R * S;
        return SRT;
    }

    void Model::processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& xform)
    {
        s32 begin_vert = vertices.count();
        if(begin_vert){
            --begin_vert;
        }

        const glm::mat3 norm_xform = glm::inverse(glm::transpose(glm::mat3(xform)));

        vertices.resize(vertices.count() + (s32)mesh->mNumVertices);
        for(u32 i = 0; i < mesh->mNumVertices; ++i)
        {
            const auto& inPos = mesh->mVertices[i];
            const auto& inNorm = mesh->mNormals[i];
    
            Vertex& vert = vertices.grow();
            vert.position.x = inPos.x;
            vert.position.y = inPos.y;
            vert.position.z = inPos.z;
            vert.position.w = 1.0f;

            vert.position = xform * vert.position;
            vert.position.w = 0.0f;

            glm::vec3 N = glm::vec3(inNorm.x, inNorm.y, inNorm.z);
            N = glm::normalize(N);
            N = glm::normalize(norm_xform * N);

            vert.normal.x = N.x;
            vert.normal.y = N.y;
            vert.normal.z = N.z;
            vert.normal.w = 0.0f;
            
            if(mesh->mTextureCoords[0]){
                const auto& inUv = mesh->mTextureCoords[0][i];
                vert.position.w = inUv.x;
                vert.normal.w = inUv.y;
            }
        }
    
        indices.resize(indices.count() + 3 * (s32)mesh->mNumFaces);
        for(u32 face = 0; face < mesh->mNumFaces; ++face)
        {
            const auto& inFace = mesh->mFaces[face];
            for(u32 i = 0; i < inFace.mNumIndices; ++i){
                indices.grow() = begin_vert + inFace.mIndices[i];
            }
        }
    }
    void Model::processNode(aiNode* node, const aiScene* scene, const glm::mat4& xform)
    {
        const glm::mat4 localXform = getTransform(node) * xform;
        for(u32 i = 0; i < node->mNumMeshes; ++i)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene, localXform);
        }
        for(u32 i = 0; i < node->mNumChildren; ++i)
        {
            processNode(node->mChildren[i], scene, localXform);
        }
    }
    void Model::parse(const char* dir)
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(dir, aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs);
        assert(scene);
        assert((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == 0);
        assert(scene->mRootNode);
        processNode(scene->mRootNode, scene, glm::scale({}, glm::vec3(0.01f)));
    }
};
