#pragma once
#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include <boost/filesystem/path.hpp>
#include <memory>
#include <stdint.h>

namespace Util { namespace Model {


class TinyObj
{
private:
    std::vector<RHI::Vertex> m_verticies;
    std::vector<uint32_t> m_indices;

public:
    explicit TinyObj(const boost::filesystem::path& objPath);
    ~TinyObj() = default;

    const std::vector<RHI::Vertex>* GetPVertices() { return &m_verticies; }
    const std::vector<uint32_t>* GetPIndices() { return &m_indices; }
};

}
}