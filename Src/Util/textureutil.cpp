#include "textureutil.h"
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace util {

texture::RawData::~RawData()
{
    FreeData();
}

void texture::RawData::FreeData()
{
    if (data)
    {
        stbi_image_free(data);
        data = nullptr;
    }
}

int texture::RawData::GetDataSize()
{
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

std::shared_ptr<util::texture::RawData> util::texture::RawData::Load(const boost::filesystem::path& texturePath, texture::RawData::Format format)
{
    std::shared_ptr<util::texture::RawData> rawData = std::make_shared<util::texture::RawData>(format);
    rawData->data = stbi_load(texturePath.string().c_str(), &rawData->width, &rawData->height, &rawData->channel, (int)format);
    return rawData->GetDataSize() != 0 ? rawData : nullptr;
}

}