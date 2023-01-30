#include "VulkanVertextInputState.h"
#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanVertextInputState::VulkanVertextInputState()
{

}


VulkanVertextInputState::~VulkanVertextInputState()
{

}

vk::PipelineVertexInputStateCreateInfo VulkanVertextInputState::GetVertexInputStateCreateInfo()
{
    auto& attributeDescs = Vertex::GetAttributeDescriptions();
    auto& bindingDesc = Vertex::GetBindingDescription();
    auto inputInfo = vk::PipelineVertexInputStateCreateInfo()
                    .setVertexAttributeDescriptions(attributeDescs)
                    .setVertexBindingDescriptions(bindingDesc); 
    return inputInfo;
}
