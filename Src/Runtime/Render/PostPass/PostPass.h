#pragma once
#include <vulkan/vulkan.hpp>
#include <Runtime/Render/RendererBase.h>

namespace Render {

class PostPass : public RendererBase
{
public:
    explicit PostPass(const RHI::VulkanInstance::Config& instanceConfig,
                const RHI::VulkanPhysicalDevice::Config& physicalConfig);
    ~PostPass() override;

protected:
    void prepare() override;
    void render() override;
};

}