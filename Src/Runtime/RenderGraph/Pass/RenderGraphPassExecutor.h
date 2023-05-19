#pragma once

#include "Runtime/RenderGraph/RenderDependencyGraph.h"
#include <vulkan/vulkan.hpp>
#include <functional>
namespace Render
{
class RenderGraphRegistry;
class IRenderGraphPassExecutor
{
public:
    using ExecuteCallback = std::function<void(RenderGraphRegistry&, vk::CommandBuffer)>;

public:
    explicit IRenderGraphPassExecutor(RenderDependencyGraph::Node* node) : m_node(node) {}
    virtual ~IRenderGraphPassExecutor() = default;
    virtual void execute(RenderGraphRegistry& registry, void* context) = 0;

protected:
    RenderDependencyGraph::Node* m_node = nullptr;
};

class RenderGraphRenderPassExecutor : public IRenderGraphPassExecutor
{
public:
    RenderGraphRenderPassExecutor(RenderDependencyGraph::Node* passNode, ExecuteCallback&& callback)
        : IRenderGraphPassExecutor(passNode), m_callback(std::move(callback))
    {}
    void execute(RenderGraphRegistry& registry, void* context) override
    {
        m_callback(registry, *static_cast<vk::CommandBuffer*>(context));
    }

protected:
    ExecuteCallback m_callback;
};

} // namespace Render