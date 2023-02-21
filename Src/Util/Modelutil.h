#pragma once

#include "Util/Textureutil.h"
#include "vulkan/vulkan_enums.hpp"
#include <assimp/material.h>
#include <assimp/scene.h>
#include <boost/filesystem/path.hpp>
#include <map>
#include <memory>
#include <stdint.h>
#include <glm/glm.hpp>

namespace Util { namespace Model {

struct VertexData
{
    glm::vec3 position;
    glm::vec2 texCoord;
    glm::vec3 normal;
    glm::vec3 tangent;
    glm::vec3 bitangent;

    static const vk::VertexInputBindingDescription& GetBindingDescription();
    static const std::array<vk::VertexInputAttributeDescription, 5>& GetAttributeDescriptions();
    bool operator==(const VertexData& r) const
    {
        return position == r.position && texCoord == r.texCoord && normal == r.normal && tangent == r.tangent && bitangent == r.bitangent;
    }
};

struct MeshData
{
    std::string name;
    std::vector<VertexData> vertices;
    std::vector<uint32_t> indices;
};

struct TextureData
{
    std::string name;
    aiTextureType type;
    aiTextureMapMode uMapMode;
    aiTextureMapMode vMapMode;
    std::shared_ptr<Util::Texture::RawData> rawData;
};

struct MaterialData
{
    std::string name;
    std::vector<TextureData> textureDatas;
    std::map<vk::ShaderStageFlagBits, boost::filesystem::path> shaderDatas;
};

struct ModelData
{
    std::vector<MeshData> meshDatas;
    std::vector<MaterialData> materialDatas;
    std::vector<size_t> materialIndexs;
};

class TinyObj
{
private:
    MeshData m_meshData;
public:
    explicit TinyObj(const boost::filesystem::path& objPath);
    ~TinyObj() = default;

    const std::vector<VertexData>* GetPVertices() { return &m_meshData.vertices; }
    const std::vector<uint32_t>* GetPIndices() { return &m_meshData.indices; }
};

class AssimpObj
{
private:
    boost::filesystem::path m_filePath;
    ModelData m_modelData;

    std::map<boost::filesystem::path, std::shared_ptr<Util::Texture::RawData>> m_textureDataMap;
public:
    explicit AssimpObj(const boost::filesystem::path& objPath);
    ~AssimpObj() = default;

    const ModelData* GetPModelData() { return &m_modelData; }
    ModelData&& MoveModelData() { return std::move(m_modelData); }
private:
    void processNode(aiNode* node, const aiScene* scene);
    void fillMeshData(Util::Model::MeshData& meshData, aiMesh* mesh);
    void fillTextureData(std::vector<Util::Model::TextureData>& textureDatas, aiMesh* mesh, aiMaterial* material, const boost::filesystem::path& textureFolderPath);

};

}
}