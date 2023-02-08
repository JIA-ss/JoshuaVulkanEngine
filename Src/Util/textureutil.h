#pragma once
#include <boost/filesystem/path.hpp>
#include <memory>

namespace util { namespace texture {


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
    Format format = Format::eRgbAlpha;
    unsigned char* data = nullptr;
public:
    explicit RawData(Format _format = Format::eRgbAlpha) : format(_format) {}
    ~RawData();

    void FreeData();
    inline int GetWidth() { return width; }
    inline int GetHeight() { return height; }
    inline int GetChannel() { return channel; }
    inline Format GetFormat() { return format; }
    inline unsigned char* GetData() { return data; }
    int GetDataSize();
public:
    static std::shared_ptr<RawData> Load(const boost::filesystem::path& texturePath, Format format);
};

}
}