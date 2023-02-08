#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <array>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
RHI_NAMESPACE_BEGIN

struct Vertex
{
    glm::vec2 position;
    glm::vec3 color;
    glm::vec2 texCoord;

    static const vk::VertexInputBindingDescription& GetBindingDescription();
    static const std::array<vk::VertexInputAttributeDescription, 3>& GetAttributeDescriptions();
};
RHI_NAMESPACE_END