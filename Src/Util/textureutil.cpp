#include "Textureutil.h"
#include "Util/Fileutil.h"
#include "vulkan/vulkan_enums.hpp"
#include <assimp/material.h>
#include <memory>
#include <ktx.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Util {

Texture::RawData::~RawData()
{
    FreeData();
}

void Texture::RawData::FreeData()
{
    if (ktxTexture)
    {
        ktxTexture_Destroy(ktxTexture);
        ktxTexture = nullptr;
        data = nullptr;
        return;
    }

    if (data)
    {
        stbi_image_free(data);
        data = nullptr;
    }
}

int Texture::RawData::GetDataSize()
{
    if (ktxTexture)
    {
        return ktxTexture_GetDataSize(ktxTexture);
    }

    switch (format)
    {
    case Format::eRgb:
    {
        // assert(channel == 3);
        return width * height * 3;
    }
    case Format::eRgbAlpha:
    {
        // assert(channel == 4);
        return width * height * 4;
    }
    default:
    {
        assert(false);
        return 0;
    }
    }
}

size_t Util::Texture::RawData::GetLevelOffset(uint32_t level, uint32_t face)
{
    if (mipLevels == 1 || !ktxTexture)
    {
        return 0;
    }

    size_t offset = 0;
    KTX_error_code ret = ktxTexture_GetImageOffset(ktxTexture, level, 0, face, &offset);
    assert(ret == KTX_SUCCESS);
    return offset;
}

std::shared_ptr<Util::Texture::RawData> Util::Texture::RawData::Load(const boost::filesystem::path& texturePath, Texture::RawData::Format format, bool cubemap,  vk::Format fmt)
{
    std::shared_ptr<Util::Texture::RawData> rawData = std::make_shared<Util::Texture::RawData>(format);
    rawData->isCubeMap = cubemap;
    rawData->vkFormat = fmt;
    if (!Util::File::fileExist(texturePath))
    {
        assert(false);
        return rawData;
    }

    std::string extension = Util::File::getLowerExtension(texturePath);
    if (extension == ".ktx")
    {
        ktxResult result = ktxTexture_CreateFromNamedFile(texturePath.string().c_str(), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &rawData->ktxTexture);
        assert(result == KTX_SUCCESS);
        rawData->width = rawData->ktxTexture->baseWidth;
        rawData->height = rawData->ktxTexture->baseHeight;
        rawData->mipLevels = rawData->ktxTexture->numLevels;
		rawData->data = ktxTexture_GetData(rawData->ktxTexture);
    }
    else
    {
        rawData->data = stbi_load(texturePath.string().c_str(), &rawData->width, &rawData->height, &rawData->channel, (int)format);
    }
    return rawData->GetDataSize() != 0 ? rawData : nullptr;
}

vk::SamplerAddressMode Util::Texture::Convert(aiTextureMapMode mapMode)
{
    switch (mapMode)
    {
    case aiTextureMapMode::aiTextureMapMode_Wrap:
    {
        return vk::SamplerAddressMode::eClampToBorder;
    }
    case aiTextureMapMode::aiTextureMapMode_Clamp:
    {
        return vk::SamplerAddressMode::eClampToEdge;
    }
    case aiTextureMapMode::aiTextureMapMode_Mirror:
    {
        return vk::SamplerAddressMode::eMirroredRepeat;
    }
    case aiTextureMapMode::aiTextureMapMode_Decal:
    {
        return vk::SamplerAddressMode::eClampToBorder;
    }
    default:
    {
        return vk::SamplerAddressMode::eRepeat;
    }
    }
}

}