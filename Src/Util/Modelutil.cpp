#include "Modelutil.h"
#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include "Util/Fileutil.h"
#include "Util/Textureutil.h"
#include <assimp/material.h>
#include <assimp/mesh.h>
#include <assimp/types.h>
#include <boost/filesystem/path.hpp>
#include <glm/fwd.hpp>
#include <iostream>
#include <stdexcept>

#include <unordered_map>
#include <vcruntime.h>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Util {

Model::TinyObj::TinyObj(const boost::filesystem::path& objPath)
{
    if (!Util::File::fileExist(objPath))
    {
        throw std::runtime_error("load model failed, File not exist: " + objPath.string() );
        return;
    }

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn,err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath.string().c_str()))
    {
        throw std::runtime_error("load model failed: " + warn + err);
    }

    std::unordered_map<RHI::Vertex, uint32_t> uniqueVertices;
    for (auto& shape : shapes)
    {
        for (auto& index : shape.mesh.indices)
        {
            RHI::Vertex vertex;

            vertex.position = glm::vec3
            {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            vertex.texCoord = glm::vec2
            {
                attrib.texcoords[2 * index.texcoord_index + 0],
                1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
            };

            if (uniqueVertices.find(vertex) == uniqueVertices.end())
            {
                uniqueVertices[vertex] = m_meshData.vertices.size();
                m_meshData.vertices.emplace_back(vertex);
            }
            m_meshData.indices.emplace_back(uniqueVertices[vertex]);
        }
    }
}


void Util::Model::AssimpObj::fillMeshData(Util::Model::MeshData& meshData, aiMesh* mesh)
{
    for (size_t i = 0; i < mesh->mNumVertices; i++)
    {
        meshData.vertices.push_back(Model::VertexData{});
        Util::Model::VertexData& vertex = meshData.vertices.back();
        glm::vec3 vec3;
        vec3.x = mesh->mVertices[i].x;
        vec3.y = mesh->mVertices[i].y;
        vec3.z = mesh->mVertices[i].z;
        vertex.position = vec3;

        vec3.x = mesh->mNormals[i].x;
        vec3.y = mesh->mNormals[i].y;
        vec3.z = mesh->mNormals[i].z;
        vertex.normal = vec3;

        if (mesh->mTextureCoords[0])
        {
            glm::vec2 vec2;
            vec2.x = mesh->mTextureCoords[0][i].x;
            vec2.y = mesh->mTextureCoords[0][i].y;
            vertex.texCoord = vec2;
        }
        else
        {
            vertex.texCoord = glm::vec2(0.0f, 0.0f);
        }

        if (mesh->mTangents)
        {
            // tangent
            vec3.x = mesh->mTangents[i].x;
            vec3.y = mesh->mTangents[i].y;
            vec3.z = mesh->mTangents[i].z;
            vertex.tangent = vec3;
        }

        if (mesh->mBitangents)
        {
            // bitangent
            vec3.x = mesh->mBitangents[i].x;
            vec3.y = mesh->mBitangents[i].y;
            vec3.z = mesh->mBitangents[i].z;
            vertex.bitangent = vec3;
        }
    }

    for (size_t i = 0; i < mesh->mNumFaces; i++)
    {
        aiFace face = mesh->mFaces[i];
        for (size_t j = 0; j < face.mNumIndices; j++)
        {
            meshData.indices.emplace_back(face.mIndices[j]);
        }
    }
}

void Util::Model::AssimpObj::fillTextureData(std::vector<Util::Model::TextureData>& textureDatas, aiMesh* mesh, aiMaterial* material, const boost::filesystem::path& textureFolderPath)
{
    for (size_t aiType = aiTextureType_DIFFUSE; aiType < aiTextureType_UNKNOWN; aiType++)
    {
        int maxTextureCount = material->GetTextureCount((aiTextureType)aiType);
        for (size_t texIdx = 0; texIdx < maxTextureCount; texIdx++)
        {
            aiString name;
            material->GetTexture((aiTextureType)aiType, texIdx, &name);

            auto texturePath = textureFolderPath / name.C_Str();
            aiTextureMapMode u_mode, v_mode;
            material->Get(AI_MATKEY_MAPPINGMODE_U(aiType, texIdx), u_mode);
            material->Get(AI_MATKEY_MAPPINGMODE_V(aiType, texIdx), v_mode);
            std::shared_ptr<Util::Texture::RawData> textureRawData;
            if (m_textureDataMap.find(texturePath) == m_textureDataMap.end())
            {
                m_textureDataMap[texturePath] = Util::Texture::RawData::Load(texturePath, Util::Texture::RawData::Format::eRgbAlpha);
            }
            textureRawData = m_textureDataMap[texturePath];
            textureDatas.emplace_back(
                Util::Model::TextureData
                {
                    name.C_Str(),
                    (aiTextureType)aiType,
                    u_mode, v_mode,
                    textureRawData
                }
            );
        }
    }
}


Model::AssimpObj::AssimpObj(const boost::filesystem::path& objPath)
    : m_filePath(objPath)
{
    Assimp::Importer assimpImporter;
    const aiScene *scene = assimpImporter.ReadFile(objPath.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        std::cout << "ERROR::ASSIMP:: " << assimpImporter.GetErrorString() << std::endl;
        return;
    }

    processNode(scene->mRootNode, scene);
}

void Model::AssimpObj::processNode(aiNode* node, const aiScene* scene)
{
    auto folder = m_filePath.parent_path();
    std::map<std::string, size_t> matNameToIdx, meshNameToIdx;
    for (size_t i = 0; i < node->mNumMeshes; i++)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        std::string meshName = mesh->mName.C_Str();
        if (meshNameToIdx.find(meshName) != meshNameToIdx.end())
        {
            continue;
        }

        MeshData meshData;
        meshData.name = meshName;
        fillMeshData(meshData, mesh);
        meshNameToIdx[meshName] = m_modelData.meshDatas.size();
        m_modelData.meshDatas.emplace_back(std::move(meshData));

        std::string matName = material->GetName().C_Str();
        if (matNameToIdx.find(matName) == matNameToIdx.end())
        {
            MaterialData materialData;
            materialData.name = matName;
            fillTextureData(materialData.textureDatas, mesh, material, folder);
            matNameToIdx[matName] = m_modelData.materialDatas.size();
            m_modelData.materialDatas.emplace_back(std::move(materialData));
        }

        m_modelData.materialIndexs.emplace_back(matNameToIdx[matName]);
    }

    for (size_t i = 0; i < node->mNumChildren; i++)
    {
        processNode(node->mChildren[i], scene);
    }
}

}