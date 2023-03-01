#pragma once
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>
#include "vulkan/vulkan_enums.hpp"


RHI_NAMESPACE_BEGIN

class VulkanRasterizationState
{
public:
private:
    bool m_DepthClampEnable;
    bool m_DiscardEnable;
    vk::PolygonMode m_PolygonMode;
    float m_LineWidth;
    vk::CullModeFlags m_CullMode;
    vk::FrontFace m_FrontFace;
    bool m_DepthBiasEnable;
public:
    VulkanRasterizationState
    (
        bool DepthClampEnable,
        bool DiscardEnable,
        vk::PolygonMode PolygonMode,
        float LineWidth,
        vk::CullModeFlags CullMode,
        vk::FrontFace FrontFace,
        bool DepthBiasEnable
    );
    ~VulkanRasterizationState();

    vk::PipelineRasterizationStateCreateInfo GetRasterizationStateCreateInfo();
};

class VulkanRasterizationStateBuilder
{
    BUILDER_SET_FUNC(VulkanRasterizationStateBuilder, bool,                 DepthClampEnable,   false)
    BUILDER_SET_FUNC(VulkanRasterizationStateBuilder, bool,                 DiscardEnable,      false)
    BUILDER_SET_FUNC(VulkanRasterizationStateBuilder, vk::PolygonMode,      PolygonMode,        vk::PolygonMode::eFill)
    BUILDER_SET_FUNC(VulkanRasterizationStateBuilder, float,                LineWidth,          1.0f)
    BUILDER_SET_FUNC(VulkanRasterizationStateBuilder, vk::CullModeFlags,    CullMode,           vk::CullModeFlagBits::eNone)
    BUILDER_SET_FUNC(VulkanRasterizationStateBuilder, vk::FrontFace,        FrontFace,          vk::FrontFace::eCounterClockwise)
    BUILDER_SET_FUNC(VulkanRasterizationStateBuilder, bool,                 DepthBiasEnable,    false)
public:
    std::shared_ptr<VulkanRasterizationState> build();
};
RHI_NAMESPACE_END