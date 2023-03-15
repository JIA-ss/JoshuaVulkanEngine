#include "PostPass.h"

using namespace Render;

PostPass::PostPass(const RHI::VulkanInstance::Config& instanceConfig,
   const RHI::VulkanPhysicalDevice::Config& physicalConfig)
   : RendererBase(instanceConfig, physicalConfig)
{

}

PostPass::~PostPass()
{
   //please implement deconstruct function
   assert(false);
}

void PostPass::prepare()
{

}

void PostPass::render()
{

}
