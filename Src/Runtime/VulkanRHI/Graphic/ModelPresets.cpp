#include "ModelPresets.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Util/Fileutil.h"
#include "Util/Modelutil.h"
#include "Util/Textureutil.h"
#include <assimp/material.h>
#include <boost/filesystem/path.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <stdint.h>

RHI_NAMESPACE_USING

static const std::vector<uint32_t> FrontPlaneIndice = { 0,1,2,2,3,0 };
static const std::vector<Util::Model::VertexData> FrontPlaneVertex =
{
    Util::Model::VertexData { glm::vec4(0,0,0,1), glm::vec4(0,0,0,0) },
    Util::Model::VertexData { glm::vec4(0,1,0,1), glm::vec4(0,1,0,0) },
    Util::Model::VertexData { glm::vec4(1,1,0,1), glm::vec4(1,1,0,0) },
    Util::Model::VertexData { glm::vec4(1,0,0,1), glm::vec4(1,0,0,0) },
};

static const std::vector<uint32_t> BackPlaneIndice = { 3,2,1,1,0,3 };
static const std::vector<Util::Model::VertexData> BackPlaneVertex =
{
    Util::Model::VertexData { glm::vec4(0,0,-1,1), glm::vec4(1,0,0,0) },
    Util::Model::VertexData { glm::vec4(0,1,-1,1), glm::vec4(1,1,0,0) },
    Util::Model::VertexData { glm::vec4(1,1,-1,1), glm::vec4(0,0,0,0) },
    Util::Model::VertexData { glm::vec4(1,0,-1,1), glm::vec4(0,1,0,0) },
};

static const std::vector<uint32_t> LeftPlaneIndice = { 3,2,1,1,0,3 };
static const std::vector<Util::Model::VertexData> LeftPlaneVertex =
{
    Util::Model::VertexData { glm::vec4(0,0,0,1), glm::vec4(0,0,0,0) },
    Util::Model::VertexData { glm::vec4(0,1,0,1), glm::vec4(0,1,0,0) },
    Util::Model::VertexData { glm::vec4(0,1,-1,1), glm::vec4(1,1,0,0) },
    Util::Model::VertexData { glm::vec4(0,0,-1,1), glm::vec4(1,0,0,0) },
};

static const std::vector<uint32_t> RightPlaneIndice = { 0,1,2,2,3,0 };
static const std::vector<Util::Model::VertexData> RightPlaneVertex =
{
    Util::Model::VertexData { glm::vec4(1,0,0,1), glm::vec4(1,0,0,0) },
    Util::Model::VertexData { glm::vec4(1,1,0,1), glm::vec4(1,1,0,0) },
    Util::Model::VertexData { glm::vec4(1,1,-1,1), glm::vec4(0,0,0,0) },
    Util::Model::VertexData { glm::vec4(1,0,-1,1), glm::vec4(0,1,0,0) },
};



std::unique_ptr<Model> ModelPresets::CreatePlaneModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout)
{
    static Util::Model::MeshData planeMeshData =
    {
        "plane", FrontPlaneVertex, FrontPlaneIndice
    };

    std::vector<Util::Model::TextureData> planeTextureData =
    {
        // diffuse
        Util::Model::TextureData
        {
            "planeDiff", aiTextureType_DIFFUSE, aiTextureMapMode_Mirror, aiTextureMapMode_Mirror,
            Util::Texture::RawData::Load(
                Util::File::getResourcePath() / "Model/Sponza-master/textures/spnza_bricks_a_diff.tga",
                Util::Texture::RawData::Format::eRgbAlpha
            )
        },
        // specular
        Util::Model::TextureData
        {
            "planeSpec", aiTextureType_SPECULAR, aiTextureMapMode_Mirror, aiTextureMapMode_Mirror,
            Util::Texture::RawData::Load(
                Util::File::getResourcePath() / "Model/Sponza-master/textures/spnza_bricks_a_ddn.tga",
                Util::Texture::RawData::Format::eRgbAlpha
            )
        }
    };

    static Util::Model::MaterialData planeMaterialData =
    {
        "plane", planeTextureData
    };

    Util::Model::ModelData planeModelData =
    {
        {planeMeshData},{planeMaterialData},{0}
    };

    return std::make_unique<Model>(device, std::move(planeModelData), layout);
}


std::unique_ptr<Model> ModelPresets::CreateCubeModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout)
{

    static Util::Model::MeshData cubeMeshData =
    {
        "cube",
        std::vector<Util::Model::VertexData>
        {
            Util::Model::VertexData { glm::vec4(0,0,0,1), glm::vec4(0,0,0,0) },
            Util::Model::VertexData { glm::vec4(0,1,0,1), glm::vec4(0,1,0,0) },
            Util::Model::VertexData { glm::vec4(1,1,0,1), glm::vec4(1,1,0,0) },
            Util::Model::VertexData { glm::vec4(1,0,0,1), glm::vec4(1,0,0,0) },

            Util::Model::VertexData { glm::vec4(0,0,-1,1), glm::vec4(1,0,0,0) },
            Util::Model::VertexData { glm::vec4(0,1,-1,1), glm::vec4(1,1,0,0) },
            Util::Model::VertexData { glm::vec4(1,1,-1,1), glm::vec4(0,0,0,0) },
            Util::Model::VertexData { glm::vec4(1,0,-1,1), glm::vec4(0,1,0,0) },
        },
        std::vector<uint32_t> {
            0,1,2,2,3,0,
            4,5,1,1,0,4,
            7,6,5,5,4,7,
            3,2,6,6,7,3,
            1,5,6,6,2,1,
            4,0,3,3,7,4
        }
    };

    static Util::Model::MaterialData cubeMaterialData =
    {
        "cube",
        std::vector<Util::Model::TextureData>
        {
            Util::Model::TextureData
            {
                "cube", aiTextureType_DIFFUSE, aiTextureMapMode_Mirror, aiTextureMapMode_Mirror,
                Util::Texture::RawData::Load(
                    Util::File::getResourcePath() / "Model/Sponza-master/textures/spnza_bricks_a_diff.tga",
                    Util::Texture::RawData::Format::eRgbAlpha
                )
            },
            Util::Model::TextureData
            {
                "cubeSpec", aiTextureType_SPECULAR, aiTextureMapMode_Mirror, aiTextureMapMode_Mirror,
                Util::Texture::RawData::Load(
                    Util::File::getResourcePath() / "Model/Sponza-master/textures/spnza_bricks_a_ddn.tga",
                    Util::Texture::RawData::Format::eRgbAlpha
                )
            }
        }
    };

    Util::Model::ModelData cubeModelData =
    {
        {cubeMeshData},{cubeMaterialData},{0}
    };

    return std::make_unique<Model>(device, std::move(cubeModelData), layout);
}

std::unique_ptr<Model> ModelPresets::CreateFrustumModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout, const Util::Math::VPMatrix& matrix)
{
    float near = matrix.GetNear();
    float far = matrix.GetFar();
    float aspect = matrix.GetAspect();
    float tanHalfFov = glm::tan(glm::radians(matrix.GetFovDegree()) * 0.5f);

    float nearPlaneTop = tanHalfFov * near;
    float nearPlaneRight = aspect * nearPlaneTop;

    float farPlaneTop = tanHalfFov * far;
    float farPlaneRight = aspect * farPlaneTop;

    std::vector<Util::Model::MeshData> frustumMeshData =
    {
        Util::Model::MeshData
        {
            "frustum",
            std::vector<Util::Model::VertexData>
            {
                // #0 origin
                Util::Model::VertexData { glm::vec4(0,0,0,1) },
                // near
                    // #1 near BottomLeft
                    Util::Model::VertexData { glm::vec4(-nearPlaneRight, -nearPlaneTop, -near, 1) },
                    // #2 near TopLeft
                    Util::Model::VertexData { glm::vec4(-nearPlaneRight, nearPlaneTop, -near, 1) },
                    // #3 near TopRight
                    Util::Model::VertexData { glm::vec4(nearPlaneRight, nearPlaneTop, -near, 1) },
                    // #4 near BottomRight
                    Util::Model::VertexData { glm::vec4(nearPlaneRight, -nearPlaneTop, -near, 1) },
                // far
                    // #5 far BottomLeft
                    Util::Model::VertexData { glm::vec4(-farPlaneRight, -farPlaneTop, -far, 1) },
                    // #6 far TopLeft
                    Util::Model::VertexData { glm::vec4(-farPlaneRight, farPlaneTop, -far, 1) },
                    // #7 far TopRight
                    Util::Model::VertexData { glm::vec4(farPlaneRight, farPlaneTop, -far, 1) },
                    // #8 far BottomRight
                    Util::Model::VertexData { glm::vec4(farPlaneRight, -farPlaneTop, -far, 1) },
            },
            std::vector<uint32_t>
            {
                // near
                1,2,3,3,4,1,
                // far
                5,6,7,7,8,5,
                // ori-near-left
                0,1,2,
                // ori-near-right
                0,3,4,
                // ori-near-top
                0,2,3,
                // ori-near-bottom
                0,4,1,
                // near-far-left
                5,6,2,2,1,5,
                // near-far-right
                4,3,7,7,8,4,
                // near-far-top
                2,6,7,7,3,2,
                // near-far-bottom
                5,1,4,4,8,5
            }
        }
    };

    std::vector<Util::Model::TextureData> planeTextureData =
    {
        // diffuse
        Util::Model::TextureData
        {
            "planeDiff", aiTextureType_DIFFUSE, aiTextureMapMode_Mirror, aiTextureMapMode_Mirror,
            Util::Texture::RawData::Load(
                Util::File::getResourcePath() / "Model/Sponza-master/textures/spnza_bricks_a_diff.tga",
                Util::Texture::RawData::Format::eRgbAlpha
            )
        },
        // specular
        Util::Model::TextureData
        {
            "planeSpec", aiTextureType_SPECULAR, aiTextureMapMode_Mirror, aiTextureMapMode_Mirror,
            Util::Texture::RawData::Load(
                Util::File::getResourcePath() / "Model/Sponza-master/textures/spnza_bricks_a_ddn.tga",
                Util::Texture::RawData::Format::eRgbAlpha
            )
        }
    };

    static Util::Model::MaterialData frustummaterialData =
    {
        "frustum", planeTextureData
    };


    Util::Model::ModelData cubeModelData =
    {
        {frustumMeshData},{frustummaterialData},{0,0,0,0}
    };

    return std::make_unique<Model>(device, std::move(cubeModelData), layout);
}



std::unique_ptr<Model> ModelPresets::CreateCerberusPBRModel(VulkanDevice* device, VulkanDescriptorSetLayout* layout)
{
    boost::filesystem::path modelPath = Util::File::getResourcePath() / "Model/Cerberus/Cerberus_LP.FBX";
    auto texturePath = modelPath.parent_path() / "Textures";
    auto assimpModel = Util::Model::AssimpObj(modelPath);
    Util::Model::ModelData&& modelData = assimpModel.MoveModelData();

    auto metallic = Util::Texture::RawData::Load(texturePath / "Cerberus_M.tga", Util::Texture::RawData::Format::eRgbAlpha);
    auto roughness = Util::Texture::RawData::Load(texturePath / "Cerberus_R.tga", Util::Texture::RawData::Format::eRgbAlpha);
    auto normal = Util::Texture::RawData::Load(texturePath / "Cerberus_N.tga", Util::Texture::RawData::Format::eRgbAlpha);
    auto ao = Util::Texture::RawData::Load(texturePath / "Raw/Cerberus_AO.tga", Util::Texture::RawData::Format::eRgbAlpha);

    std::vector<Util::Model::TextureData>& texDatas = modelData.materialDatas[0].textureDatas;
    // Binding1: Albedo
    texDatas[0].name = "Albedo";
    // Binding2: Metallic
    texDatas.push_back(Util::Model::TextureData{
        "Metallic",
        aiTextureType_METALNESS,
        aiTextureMapMode_Clamp, aiTextureMapMode_Clamp,
        metallic
    });
    // Binding3: Roughness
    texDatas.push_back(Util::Model::TextureData{
        "Roughness",
        aiTextureType_METALNESS,
        aiTextureMapMode_Clamp, aiTextureMapMode_Clamp,
        roughness
    });
    // Binding4: Normal
    texDatas.push_back(Util::Model::TextureData{
        "Normal",
        aiTextureType_METALNESS,
        aiTextureMapMode_Clamp, aiTextureMapMode_Clamp,
        normal
    });
    // Binding5: AO
    texDatas.push_back(Util::Model::TextureData{
        "AO",
        aiTextureType_METALNESS,
        aiTextureMapMode_Clamp, aiTextureMapMode_Clamp,
        ao
    });
    std::unique_ptr<Model> model = std::make_unique<Model>(device, std::move(modelData), layout);
    return model;
}
