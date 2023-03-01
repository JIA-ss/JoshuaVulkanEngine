#pragma once
#include <vulkan/vulkan.hpp>

#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "vulkan/vulkan_structs.hpp"

RHI_NAMESPACE_BEGIN

class VulkanDepthStencilState
{
public:
    struct Config
    {
        bool DepthTestEnable = VK_TRUE;
        bool DepthWriteEnable = VK_TRUE;
        vk::CompareOp DepthCompareOp = vk::CompareOp::eLess;
        bool DepthBoundsTestEnable = VK_FALSE;
        float MinDepthBounds = 0.0f;
        float MaxDepthBounds = 1.0f;
        bool StencilTestEnable = VK_FALSE;
        static const Config DEFAULT;
    };
private:
    Config m_config;
public:
    VulkanDepthStencilState(const Config& config = Config::DEFAULT);
    ~VulkanDepthStencilState();

    vk::PipelineDepthStencilStateCreateInfo GetDepthStencilStateCreateInfo();
};
RHI_NAMESPACE_END