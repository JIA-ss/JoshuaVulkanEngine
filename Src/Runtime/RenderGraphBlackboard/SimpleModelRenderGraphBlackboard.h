#pragma once

#include "Runtime/RenderGraph/Model/ModelWrapper.h"
#include "Runtime/RenderGraph/RenderGraph.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include "Runtime/VulkanRHI/VulkanInstance.h"
#include "Runtime/VulkanRHI/VulkanPhysicalDevice.h"
#include <memory>

namespace Render
{
class SimpleModelRenderGraphBlackboard
{
public:
    SimpleModelRenderGraphBlackboard(
        const RHI::VulkanInstance::Config& instanceConfig, const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    virtual ~SimpleModelRenderGraphBlackboard();
    virtual void buildRenderGraph();
    virtual void executeRenderGraph();

protected:
    std::unique_ptr<RHI::VulkanInstance> m_pInstance;
    std::unique_ptr<RHI::VulkanPhysicalDevice> m_pPhysicalDevice;
    std::unique_ptr<RHI::VulkanDevice> m_pDevice;

    std::unique_ptr<ModelWrapper> m_pModelWrapper;
    std::unique_ptr<RenderGraph> m_pRenderGraph;
};
} // namespace Render