#include "PostPass.h"

using namespace Render;


PostPass::PostPass(RHI::VulkanDevice* device, uint32_t fbWidth, uint32_t fbHeight)
   : m_pDevice(device)
   , m_width(fbWidth)
   , m_height(fbHeight)
{

}

void PostPass::Prepare()
{
      prepareLayout();
      {
         std::vector<RHI::VulkanFramebuffer::Attachment> attachments;
         prepareAttachments(attachments);
         prepareRenderPass(attachments);
         prepareFramebuffer(std::move(attachments));
      }
      preparePipeline();
      prepareOutputDescriptorSets();
}

void PostPass::prepareLayout()
{
   m_pPipelineLayout.reset(
   new RHI::VulkanPipelineLayout(
      m_pDevice,
      {m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER}
      , {}
      )
   );
}