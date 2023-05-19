#include "RenderGraphRegistry.h"
#include "Runtime/RenderGraph/Pass/RenderGraphPassNode.h"
#include <string.h>

using namespace Render;

RenderGraphPassNode* RenderGraphRegistry::getPassNode(const char* name) const
{
    for (auto& passNode : m_renderGraph->m_passNodes)
    {
        if (passNode && strcmp(name, passNode->getName()) == 0)
        {
            return passNode.get();
        }
    }
    return nullptr;
}

RenderGraphHandleBase RenderGraphRegistry::getResourceHandle(const char* name) const
{
    for (auto& resourceNode : m_renderGraph->m_resourceNodes)
    {
        if (resourceNode && strcmp(name, resourceNode->getName()) == 0)
        {
            return resourceNode->getHandle();
        }
    }
    return RenderGraphHandleBase::InValid;
}

RenderGraphResourceNode* RenderGraphRegistry::getResourceNode(const RenderGraphHandleBase& handle) const
{
    if (handle.isValid())
    {
        return m_renderGraph->m_resourceNodes[handle.getId()].get();
    }
    assert(false);
    return nullptr;
}
RenderGraphVirtualResourceBase* RenderGraphRegistry::getVirtualResource(const RenderGraphHandleBase& handle) const
{
    if (handle.isValid())
    {
        return m_renderGraph->m_virtualResources[handle.getId()].get();
    }
    assert(false);
    return nullptr;
}

RenderGraphHandle<RHI::VulkanImageResource> RenderGraphRegistry::getPresentImageHandle() const {}

void RenderGraphRegistry::registerPassNode(RenderGraphPassNode* passNode) { registerRDGNode(passNode); }
void RenderGraphRegistry::registerPassReadingDependency(
    RenderGraphPassNode* passNode, const RenderGraphHandleBase& resourceHandle)
{
    assert(passNode);
    /*
        passNode --write-> resourceNode --read-> passNode
    */
    m_renderGraph->m_dependencyGraph.addEdge(getResourceNode(resourceHandle), passNode);
}
void RenderGraphRegistry::registerPassWritingDependency(
    RenderGraphPassNode* passNode, const RenderGraphHandleBase& resourceHandle)
{
    assert(passNode);
    /*
        passNode --write-> resourceNode --read-> passNode
    */
    m_renderGraph->m_dependencyGraph.addEdge(passNode, getResourceNode(resourceHandle));
}