#include "VulkanColorBlendState.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_enums.hpp"
#include "vulkan/vulkan_structs.hpp"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

VulkanColorBlendState::VulkanColorBlendState()
{

    /*
    if (blendEnable)
    {
        finalColor.rgb = (srcColorBlendFactor * newColor.rgb) <colorBlendOp> (dstColorBlendFactor * oldColor.rgb);
        finalColor.a = (srcAlphaBlendFactor * newColor.a) <alphaBlendOp> (dstAlphaBlendFactor * oldColor.a);
    }
    else
    {
        finalColor = newColor;
    }

    finalColor = finalColor & colorWriteMask;
    */

    /*
        alpha blending:
        finalColor.rgb = newColor.rgb * newAlpha + (1 - newAlpha) * oldColor
        finalColor.a = new Alpha.a
    */

    m_vkColorBlendAttachmentState.setColorWriteMask(vk::ColorComponentFlagBits::eR |
                                                    vk::ColorComponentFlagBits::eG |
                                                    vk::ColorComponentFlagBits::eB |
                                                    vk::ColorComponentFlagBits::eA)
                                .setBlendEnable(true)
                                .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
                                .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
                                .setColorBlendOp(vk::BlendOp::eAdd)
                                .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
                                .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
                                .setAlphaBlendOp(vk::BlendOp::eAdd);
}


VulkanColorBlendState::~VulkanColorBlendState()
{

}

vk::PipelineColorBlendStateCreateInfo VulkanColorBlendState::GetColorBlendStateCreateInfo()
{
    auto createInfo = vk::PipelineColorBlendStateCreateInfo()
                    .setAttachments(m_vkColorBlendAttachmentState)
                    .setLogicOpEnable(false);
    return createInfo;
}