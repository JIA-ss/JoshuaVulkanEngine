#include "SimpleModelRenderGraphBlackboard.h"
#include "Runtime/RenderGraph/Model/ModelWrapper.h"
#include "Runtime/RenderGraph/RenderGraph.h"
#include "Runtime/RenderGraph/RenderGraphBuilder.h"
#include "Runtime/RenderGraph/Pass/RenderGraphPassNode.h"
#include "Runtime/RenderGraph/Resource/RenderGraphResourceNode.h"
#include "Util/Fileutil.h"

using namespace Render;

SimpleModelRenderGraphBlackboard::SimpleModelRenderGraphBlackboard(
    const RHI::VulkanInstance::Config& instanceConfig, const RHI::VulkanPhysicalDevice::Config& physicalConfig)
{
    m_pInstance.reset(new RHI::VulkanInstance(instanceConfig));
    m_pPhysicalDevice.reset(new RHI::VulkanPhysicalDevice(physicalConfig, m_pInstance.get()));
    m_pDevice.reset(new RHI::VulkanDevice(m_pPhysicalDevice.get()));
    m_pRenderGraph.reset(new RenderGraph(m_pDevice.get()));
}

SimpleModelRenderGraphBlackboard::~SimpleModelRenderGraphBlackboard()
{
    m_pRenderGraph.reset();
    m_pDevice.reset();
    m_pPhysicalDevice.reset();
    m_pInstance.reset();
}

void SimpleModelRenderGraphBlackboard::buildRenderGraph()
{
    RenderGraphBuilder builder(m_pRenderGraph.get(), nullptr);
    RenderGraphRegistry registry(m_pRenderGraph.get(), nullptr);

    m_pModelWrapper.reset(
        new ModelWrapper(m_pDevice.get(), Util::File::getResourcePath() / "Model" / "nanosuit" / "nanosuit.obj"));

    auto passNode = builder.addPassNode(
        "present",
        [=](RenderGraphBuilder& builder, RenderGraphRegistry& registry) -> IRenderGraphPassExecutor::ExecuteCallback&&
        {
            auto passNode = builder.getCurrentPassNode();

            m_pModelWrapper->registerToRenderGraph(builder);
            m_pModelWrapper->uploadBuffers(registry);
            m_pModelWrapper->updateUniformBuffer(registry);

            // set binding info
            RHI::VulkanPipelineLayout::BindingInfo bindingInfo;
            bindingInfo.setBindings.resize(2);
            bindingInfo.setBindings[0] = m_pDevice->GetDescLayoutPresets().UBO->GetVkBindings();
            bindingInfo.setBindings[1] = m_pDevice->GetDescLayoutPresets().CUSTOM5SAMPLER->GetVkBindings();
            passNode->setBindingInfo(bindingInfo);

            // set attachments
            passNode->setAttachments();
            passNode->setAttachmentMeta();
            passNode->setSubpassDescription();
            passNode->setSubpassDependencies();
        });

    m_pRenderGraph->compile();
}

void SimpleModelRenderGraphBlackboard::executeRenderGraph()
{
    while (!m_pPhysicalDevice->GetPWindow()->ShouldClose())
    {
        m_pPhysicalDevice->GetPWindow()->PollWindowEvent();
        m_pRenderGraph->execute();
    }
}