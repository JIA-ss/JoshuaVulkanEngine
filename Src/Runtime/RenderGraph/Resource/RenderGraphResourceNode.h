#pragma once

#include "Runtime/RenderGraph/RenderDependencyGraph.h"
#include "Runtime/RenderGraph/Resource/RenderGraphHandle.h"
namespace Render
{

class RenderGraphBuilder;
class RenderGraphRegistry;
class RenderGraphPassNode;
class RenderGraphVirtualResourceBase;
class RenderGraphResourceNode : public RenderDependencyGraph::Node
{
    friend class RenderGraphBuilder;
    friend class RenderGraphRegistry;

public:
    explicit RenderGraphResourceNode(const char* name, RenderGraphHandleBase handle) : m_name(name), m_handle(handle) {}
    inline const char* getName() const { return m_name; }
    inline RenderGraphHandleBase getHandle() const { return m_handle; }
    inline RenderGraphVirtualResourceBase* getVirtualResource() const { return m_virtualResource; }

protected:
    const char* m_name = nullptr;
    RenderGraphHandleBase m_handle = RenderGraphHandleBase::InValid;
    RenderGraphVirtualResourceBase* m_virtualResource = nullptr;
};

} // namespace Render