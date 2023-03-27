#include "PrePass.h"

#include "GeometryPrePass.h"
using namespace Render;


std::unique_ptr<PrePass> PrePass::CreateGeometryPrePass(RHI::VulkanDevice* device, Camera* camera, uint32_t fbWidth, uint32_t fbHeight)
{
    return std::make_unique<GeometryPrePass>(device, camera, fbWidth, fbHeight);
}


void PrePass::prepareLayout()
{
    m_pPipelineLayout.reset(
        new RHI::VulkanPipelineLayout(
            m_pDevice,
            {m_pDevice->GetDescLayoutPresets().UBO, m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER}
            , {}
            )
        );
}