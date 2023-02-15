#include "VulkanRasterizationState.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include <vulkan/vulkan.hpp>

RHI_NAMESPACE_USING

std::shared_ptr<VulkanRasterizationState> VulkanRasterizationStateBuilder::build()
{
    return std::make_shared<VulkanRasterizationState>(
            m_DepthClampEnable
        , m_DiscardEnable
        , m_PolygonMode
        , m_LineWidth
        , m_CullMode
        , m_FrontFace
        , m_DepthBiasEnable
    );
}

VulkanRasterizationState::VulkanRasterizationState(
    bool DepthClampEnable,
    bool DiscardEnable,
    vk::PolygonMode PolygonMode,
    float LineWidth,
    vk::CullModeFlags CullMode,
    vk::FrontFace FrontFace,
    bool DepthBiasEnable
)
: m_DepthClampEnable(DepthClampEnable)
, m_DiscardEnable(DiscardEnable)
, m_PolygonMode(PolygonMode)
, m_LineWidth(LineWidth)
, m_CullMode(CullMode)
, m_FrontFace(FrontFace)
, m_DepthBiasEnable(DepthBiasEnable)
{

}


VulkanRasterizationState::~VulkanRasterizationState()
{

}

vk::PipelineRasterizationStateCreateInfo VulkanRasterizationState::GetRasterizationStateCreateInfo()
{
    auto createInfo = vk::PipelineRasterizationStateCreateInfo()
                    .setDepthClampEnable(m_DepthClampEnable)
                    .setRasterizerDiscardEnable(m_DiscardEnable)
                    .setPolygonMode(m_PolygonMode)
                    .setLineWidth(m_LineWidth)
                    .setCullMode(m_CullMode)
                    .setFrontFace(m_FrontFace)
                    .setDepthBiasEnable(m_DepthBiasEnable);
    return createInfo;
}