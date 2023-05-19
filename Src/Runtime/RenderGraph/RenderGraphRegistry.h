#pragma once

#include "Runtime/RenderGraph/Resource/RenderGraphHandle.h"
#include "Runtime/RenderGraph/Resource/RenderGraphResourceNode.h"
#include "Runtime/RenderGraph/Resource/RenderGraphVirtualResource.h"
#include "Runtime/RenderGraph/RenderGraph.h"
#include "Runtime/VulkanRHI/Resources/VulkanImage.h"
namespace Render
{

class RenderGraphPassNode;
class RenderGraphBuilder;
class RenderGraphRegistry
{
    friend class RenderGraphBuilder;

public:
    RenderGraphRegistry(RenderGraph* renderGraph, RenderGraphPassNode* passNode)
        : m_renderGraph(renderGraph), m_currentPassNode(passNode)
    {}

    RenderGraphPassNode* getPassNode(const char* name) const;
    RenderGraphHandleBase getResourceHandle(const char* name) const;
    RenderGraphResourceNode* getResourceNode(const RenderGraphHandleBase& handle) const;
    RenderGraphVirtualResourceBase* getVirtualResource(const RenderGraphHandleBase& handle) const;

    RenderGraphHandle<RHI::VulkanImageResource> getPresentImageHandle() const;

    template <typename ResourceType>
    RenderGraphVirtualResource<ResourceType>* getVirtualResource(const RenderGraphHandle<ResourceType>& handle) const
    {
        return static_cast<RenderGraphVirtualResource<ResourceType>*>(
            getVirtualResource(RenderGraphHandleBase(handle.getId())));
    }

private:
    void registerPassNode(RenderGraphPassNode* passNode);
    void registerPassReadingDependency(RenderGraphPassNode* passNode, const RenderGraphHandleBase& resourceHandle);
    void registerPassWritingDependency(RenderGraphPassNode* passNode, const RenderGraphHandleBase& resourceHandle);
    template <typename ResourceType>
    RenderGraphHandle<ResourceType> registerResourceNode(
        const char* name, const RenderGraphVirtualResourceBase::Description& description,
        std::unique_ptr<ResourceType> resource)
    {
        RenderGraphResource<ResourceType>* rgResource = registerRenderGraphResource(description, std::move(resource));
        return registerResourceNode(name, rgResource);
    }

    template <typename ResourceType>
    RenderGraphResource<ResourceType>* registerRenderGraphResource(
        const RenderGraphVirtualResourceBase::Description& description, std::unique_ptr<ResourceType> resource)
    {
        m_renderGraph->m_resources.emplace_back(
            std::make_unique<RenderGraphResource<ResourceType>>(description, std::move(resource)));

        return static_cast<RenderGraphResource<ResourceType>*>(m_renderGraph->m_resources.back().get());
    }

    template <typename ResourceType>
    RenderGraphHandle<ResourceType>
    registerResourceNode(const char* name, RenderGraphResource<ResourceType>* rgResource)
    {
        if (auto handle = getResourceHandle(name); handle.isValid())
        {
            RenderGraphVirtualResource<ResourceType>* vResource =
                getVirtualResource(RenderGraphHandle<ResourceType>(handle));
            // name dumplicate
            assert(vResource->getResourceRef()->getRGResource() == rgResource);
            return RenderGraphHandle<ResourceType>(handle);
        }

        if (!rgResource)
        {
            // register rgResource first
            assert(false);
            return RenderGraphHandle<ResourceType>(RenderGraphHandleBase::InValid);
        }

        // target handle
        RenderGraphHandleBase handle(m_renderGraph->m_resourceNodes.size());
        // create resource node
        auto resourceNode = std::make_unique<RenderGraphResourceNode>(name, handle);
        // create virtual resource
        auto virtualResource =
            std::make_unique<RenderGraphVirtualResource<ResourceType>>(resourceNode.get(), rgResource);

        // register resource node to rdg
        registerRDGNode(resourceNode.get());

        // register resource node and virtual resource
        resourceNode->m_virtualResource = virtualResource.get();
        m_renderGraph->m_resourceNodes.emplace_back(std::move(resourceNode));
        m_renderGraph->m_virtualResources.emplace_back(std::move(virtualResource));

        return handle;
    }

    inline void registerRDGNode(RenderDependencyGraph::Node* rdgNode)
    {
        m_renderGraph->m_dependencyGraph.addNode(rdgNode);
    }

private:
    RenderGraph* m_renderGraph;
    RenderGraphPassNode* m_currentPassNode;
};

} // namespace Render