#pragma once
#include <boost/filesystem/path.hpp>
#include <vulkan/vulkan.hpp>
#include <assimp/material.h>
#include <memory>
#include <ktx.h>
namespace Util { namespace Texture {


class RawData final
{
public:
    enum class Format
    {
        eDefault    = 0, // only used for desired_channels
        eGrey       = 1,
        eGreyAlpha  = 2,
        eRgb        = 3,
        eRgbAlpha   = 4
    };
private:
    int width = 0;
    int height = 0;
    int channel = 0;
    int mipLevels = 1;
    Format format = Format::eRgbAlpha;
    unsigned char* data = nullptr;
    ktxTexture* ktxTexture = nullptr;
    bool isCubeMap = false;
    vk::Format vkFormat = vk::Format::eR8G8B8A8Unorm;
public:
    explicit RawData(Format _format = Format::eRgbAlpha) : format(_format) {}
    ~RawData();

    void FreeData();
    inline int GetWidth() { return width; }
    inline int GetHeight() { return height; }
    inline int GetChannel() { return channel; }
    inline int GetMipLevels() { return mipLevels; }
    inline Format GetFormat() { return format; }
    inline vk::Format GetVkFormat() { return vkFormat; }
    inline unsigned char* GetData() { return data; }
    inline bool IsCubeMap() { return isCubeMap; }
    int GetDataSize();
    size_t GetLevelOffset(uint32_t level, uint32_t face);
public:
    static std::shared_ptr<RawData> Load(const boost::filesystem::path& texturePath, Format format, bool cubemap = false, vk::Format fmt = vk::Format::eR8G8B8A8Unorm);
};

    vk::SamplerAddressMode Convert(aiTextureMapMode mapMode);
}
}