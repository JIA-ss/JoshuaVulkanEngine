#pragma once
#include "Runtime/Render/PostPass/PresentPostPass.h"

namespace Render {

class LinkedListColorPass : public PresetPostPass
{
public:
    explicit LinkedListColorPass(
        RHI::VulkanDevice* device,
        Camera* cam,
        Lights* light,
        uint32_t fbWidth, uint32_t fbHeight,
        std::shared_ptr<RHI::VulkanShaderSet> shaderSet,
        const std::vector<RHI::VulkanFramebuffer::Attachment>& attachments,
        std::shared_ptr<RHI::VulkanDescriptorSetLayout> layout
        )
    : PresetPostPass(device, cam, light, fbWidth, fbHeight, shaderSet, attachments)
    , m_pLayout(layout)
    { }
    ~LinkedListColorPass() override = default;

protected:
    void prepareLayout() override;
private:
    std::shared_ptr<RHI::VulkanDescriptorSetLayout> m_pLayout;
};

}