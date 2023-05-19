#pragma once

#include "Runtime/RenderGraph/Resource/RenderGraphResource.h"
#include "Runtime/RenderGraph/Resource/RenderGraphResourceNode.h"
namespace Render
{

class RenderGraphVirtualResourceBase
{
public:
    struct Description
    {
        vk::DescriptorType type;
        uint32_t setId;
        uint32_t bindingId;
        std::optional<uint32_t> bufferRange;
        std::optional<uint32_t> bufferOffset;
    };

public:
    explicit RenderGraphVirtualResourceBase(
        RenderGraphResourceNode* resourceNode, std::unique_ptr<RenderGraphResourceBaseRef> resourceRef)
        : m_resourceNode(resourceNode), m_resourceRef(std::move(resourceRef))
    {}
    virtual ~RenderGraphVirtualResourceBase() = default;

    inline RenderGraphResourceNode* getResourceNode() const { return m_resourceNode; }
    virtual RenderGraphResourceBaseRef* getResourceRef() const { return m_resourceRef.get(); }

    inline void setDescription(const Description& description) { m_description = description; }
    inline const Description& getDescription() const { return m_description; }

    inline bool isReleased() const { return m_resourceRef == nullptr; }
    inline void releaseResource() { m_resourceRef.reset(); }

protected:
    RenderGraphResourceNode* m_resourceNode;
    Description m_description;
    std::unique_ptr<RenderGraphResourceBaseRef> m_resourceRef;
};

template <typename ResourceType> class RenderGraphVirtualResource : public RenderGraphVirtualResourceBase
{
public:
    using Type = ResourceType;
    explicit RenderGraphVirtualResource(
        RenderGraphResourceNode* resourceNode, std::unique_ptr<RenderGraphResourceBaseRef> resourceRef)
        : RenderGraphVirtualResourceBase(resourceNode, std::move(resourceRef))
    {}
    explicit RenderGraphVirtualResource(RenderGraphResourceNode* resourceNode, RenderGraphResourceBase* resource)
        : RenderGraphVirtualResourceBase(resourceNode, std::make_unique<RenderGraphResourceRef<ResourceType>>(resource))
    {}

    RenderGraphResourceRef<ResourceType>* getResourceRef() const override
    {
        return static_cast<RenderGraphResourceRef<ResourceType>*>(RenderGraphVirtualResourceBase::getResourceRef());
    }
};

}; // namespace Render