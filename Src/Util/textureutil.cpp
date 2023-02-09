#include "Textureutil.h"
#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Util {

Texture::RawData::~RawData()
{
    FreeData();
}

void Texture::RawData::FreeData()
{
    if (data)
    {
        stbi_image_free(data);
        data = nullptr;
    }
}

int Texture::RawData::GetDataSize()
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

std::shared_ptr<Util::Texture::RawData> Util::Texture::RawData::Load(const boost::filesystem::path& texturePath, Texture::RawData::Format format)
{
    std::shared_ptr<Util::Texture::RawData> rawData = std::make_shared<Util::Texture::RawData>(format);
    rawData->data = stbi_load(texturePath.string().c_str(), &rawData->width, &rawData->height, &rawData->channel, (int)format);
    return rawData->GetDataSize() != 0 ? rawData : nullptr;
}

}