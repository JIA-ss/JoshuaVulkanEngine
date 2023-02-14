#pragma once
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/RendererBase.h>

namespace Render {

class SimpleModelRenderer : public RendererBase
{
protected:
    std::vector<RHI::Vertex> m_vertices;
    std::unique_ptr<RHI::VulkanVertexBuffer> m_pVulkanVertexBuffer;

    std::vector<uint32_t> m_indices;
    std::unique_ptr<RHI::VulkanVertexIndexBuffer> m_pVulkanVertexIndexBuffer;

public:
    explicit SimpleModelRenderer(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    ~SimpleModelRenderer() override;

protected:
    void prepare() override;
    void render() override;
};

}