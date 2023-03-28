#include "LinkedListColorPass.h"

using namespace Render;

void LinkedListColorPass::prepareLayout()
{
    m_pPipelineLayout.reset(
    new RHI::VulkanPipelineLayout(
        m_pDevice,
        {m_pDevice->GetDescLayoutPresets().UBO, m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER, m_pLayout}
        , {}
        )
    );
}