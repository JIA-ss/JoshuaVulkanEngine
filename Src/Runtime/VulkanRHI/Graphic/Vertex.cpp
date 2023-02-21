#include "Vertex.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_structs.hpp"
#include <array>
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

const vk::VertexInputBindingDescription& Vertex::GetBindingDescription()
{
    static auto bindingDesc = vk::VertexInputBindingDescription()
                    .setBinding(0)
                    .setStride(sizeof(Vertex))
                    .setInputRate(vk::VertexInputRate::eVertex);
    return bindingDesc;
}

const std::array<vk::VertexInputAttributeDescription,5>& Vertex::GetAttributeDescriptions()
{
    static std::array<vk::VertexInputAttributeDescription, 5> attributeDescs{};
    attributeDescs[0]
                    .setBinding(0)
                    .setLocation(0)
                    .setFormat(vk::Format::eR32G32B32Sfloat)
                    .setOffset(offsetof(Vertex, position));

    attributeDescs[1]
                    .setBinding(0)
                    .setLocation(1)
                    .setFormat(vk::Format::eR32G32Sfloat)
                    .setOffset(offsetof(Vertex, texCoord));

    attributeDescs[2]
                    .setBinding(0)
                    .setLocation(2)
                    .setFormat(vk::Format::eR32G32B32Sfloat)
                    .setOffset(offsetof(Vertex, normal));

    attributeDescs[3]
                    .setBinding(0)
                    .setLocation(3)
                    .setFormat(vk::Format::eR32G32B32Sfloat)
                    .setOffset(offsetof(Vertex, tangent));

    attributeDescs[4]
                    .setBinding(0)
                    .setLocation(4)
                    .setFormat(vk::Format::eR32G32B32Sfloat)
                    .setOffset(offsetof(Vertex, bitangent));

    return attributeDescs;
}