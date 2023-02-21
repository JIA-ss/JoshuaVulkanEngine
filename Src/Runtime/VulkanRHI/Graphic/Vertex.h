#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Util/Modelutil.h"
#include <array>
#include <functional>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
RHI_NAMESPACE_BEGIN

using Vertex = Util::Model::VertexData;


RHI_NAMESPACE_END

namespace std {

template<> struct hash<RHI::Vertex>
{
    size_t operator()(RHI::Vertex const& vertex) const
    {
        return
            hash<glm::vec3>()(vertex.position) ^ (hash<glm::vec2>()(vertex.texCoord) << 1) >> 1
            ^ ((hash<glm::vec2>()(vertex.texCoord) << 1) >> 1)
            ^ ((hash<glm::vec3>()(vertex.normal) << 1) >> 1)
            ^ ((hash<glm::vec3>()(vertex.tangent) << 1) >> 1)
            ^ ((hash<glm::vec3>()(vertex.bitangent) << 1) >> 1);
    }
};

}