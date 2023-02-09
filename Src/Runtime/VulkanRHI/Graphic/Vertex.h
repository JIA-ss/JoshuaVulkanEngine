#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <array>
#include <functional>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
RHI_NAMESPACE_BEGIN

struct Vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const { return position == other.position && color == other.color && texCoord == other.texCoord; }

    static const vk::VertexInputBindingDescription& GetBindingDescription();
    static const std::array<vk::VertexInputAttributeDescription, 3>& GetAttributeDescriptions();
};

RHI_NAMESPACE_END

namespace std {

template<> struct hash<RHI::Vertex>
{
    size_t operator()(RHI::Vertex const& vertex) const
    {
        return
            (
                (
                    hash<glm::vec3>()(vertex.position)
                    ^ (hash<glm::vec3>()(vertex.color) << 1)
                ) >> 1
            ) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
    }
};

}