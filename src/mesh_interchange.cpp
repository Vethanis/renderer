
#define GLM_SWIZZLE
#include "mesh_interchange.h"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

#include <glm/gtx/transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace mesh_interchange{

    glm::mat4 getTransform(aiNode* node){
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

    void Model::processMesh(aiMesh* mesh, const aiScene* scene, const glm::mat4& xform, 
        Geometry& out, Vector<HashString>& mat_ids){

        s32 begin_vert = out.vertices.count();
        if(begin_vert){
            --begin_vert;
        }

        HashString matid;
        if(mesh->mMaterialIndex >= 0 && mesh->mMaterialIndex < scene->mNumMaterials){
            aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
            const u32 numDiffuseTextures = material->GetTextureCount(aiTextureType_DIFFUSE);
            const u32 numNormalTextures = material->GetTextureCount(aiTextureType_NORMALS);
    
            Material mat;

            mat.albedo.resize((s32)numDiffuseTextures);
            mat.normal.resize((s32)numNormalTextures);
    
            for(u32 i = 0; i < numDiffuseTextures; ++i){
                aiString str;
                material->GetTexture(aiTextureType_DIFFUSE, i, &str);
                if(str.C_Str()){
                    mat.albedo.grow() = str.C_Str();
                }
            }
    
            for(u32 i = 0; i < numNormalTextures; ++i){
                aiString str;
                material->GetTexture(aiTextureType_NORMALS, i, &str);
                if(str.C_Str()){
                    mat.normal.grow() = str.C_Str();
                }
            }
    
            matid = mat.id();
    
            if(mat_ids.find(matid) == -1){
                mat_ids.grow() = matid;
                materials.grow() = mat;
            }
        }
        s32 mat_loc = mat_ids.find(matid);
        mat_loc = glm::clamp(mat_loc, 0, MAX_MATERIALS_PER_MESH - 1);

        const glm::mat3 norm_xform = glm::inverse(glm::transpose(glm::mat3(xform)));

        out.vertices.resize(out.vertices.count() + (s32)mesh->mNumVertices);
        for(u32 i = 0; i < mesh->mNumVertices; ++i){
            const auto& inPos = mesh->mVertices[i];
            const auto& inNorm = mesh->mNormals[i];
    
            Vertex& vert = out.vertices.grow();
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
            vert.matid = (u32)mat_loc;
            
            if(mesh->mTextureCoords[0]){
                const auto& inUv = mesh->mTextureCoords[0][i];
                vert.position.w = inUv.x;
                vert.normal.w = inUv.y;
            }
        }
    
        out.indices.resize(out.indices.count() + 3 * (s32)mesh->mNumFaces);
        for(u32 face = 0; face < mesh->mNumFaces; ++face){
            const auto& inFace = mesh->mFaces[face];
            for(u32 i = 0; i < inFace.mNumIndices; ++i){
                out.indices.grow() = begin_vert + inFace.mIndices[i];
            }
            const u32 ia = out.indices[out.indices.count() - 3];
            const u32 ib = out.indices[out.indices.count() - 2];
            const u32 ic = out.indices[out.indices.count() - 1];

            Vertex& va = out.vertices[ia];
            Vertex& vb = out.vertices[ib];
            Vertex& vc = out.vertices[ic];

            const glm::vec3 e1 = vb.position.xyz() - va.position.xyz();
            const glm::vec3 e2 = vc.position.xyz() - va.position.xyz();
            const glm::vec2 duv1 = glm::vec2(vb.position.w, vb.normal.w) - glm::vec2(va.position.w, va.normal.w);
            const glm::vec2 duv2 = glm::vec2(vc.position.w, vc.normal.w) - glm::vec2(va.position.w, va.normal.w);


            glm::vec3 t;
            const float f = 1.0f / (duv1.x * duv2.y - duv2.x * duv1.y);

            t.x = f * (duv2.y * e1.x - duv1.y * e2.x);
            t.y = f * (duv2.y * e1.y - duv1.y * e2.y);
            t.z = f * (duv2.y * e1.z - duv1.y * e2.z);
            t = glm::normalize(t);

            va.tangent = t;
            vb.tangent = t;
            vc.tangent = t;
        }
    }
    void Model::processNode(aiNode* node, const aiScene* scene, 
        const glm::mat4& xform, Vector<HashString>& mat_ids){

        const glm::mat4 localXform = getTransform(node) * xform;
        for(u32 i = 0; i < node->mNumMeshes; ++i){
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            processMesh(mesh, scene, localXform, meshes, mat_ids);
        }
        for(u32 i = 0; i < node->mNumChildren; ++i){
            processNode(node->mChildren[i], scene, localXform, mat_ids);
        }
    }
    void Model::parse(const char* dir){
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(dir, aiProcessPreset_TargetRealtime_Quality | aiProcess_FlipUVs);
        assert(scene);
        assert((scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) == 0);
        assert(scene->mRootNode);
    
        Vector<HashString> mat_ids;

        processNode(scene->mRootNode, scene, glm::scale({}, glm::vec3(0.01f)), mat_ids);
    }
};
