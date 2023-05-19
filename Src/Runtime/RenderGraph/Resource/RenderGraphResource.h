#pragma once

#include <atomic>
#include <client/TracyLock.hpp>
#include <memory>
#include <optional>
#include <stdint.h>
#include <assert.h>
#include <type_traits>
#include "RenderGraphHandle.h"
#include <vulkan/vulkan.hpp>

namespace Render
{
class RenderGraphResourceBaseRef;
class RenderGraphResourceBase
{
    friend class RenderGraphResourceBaseRef;

public:
    explicit RenderGraphResourceBase() {}
    virtual ~RenderGraphResourceBase() = default;

    inline uint16_t getRefCount() const { return m_refCount.load(std::memory_order_acquire); }

protected:
    inline void incrementRef() { m_refCount.fetch_add(1, std::memory_order_release); }
    inline void decrementRef()
    {
        uint16_t beforeValue = m_refCount.fetch_sub(1, std::memory_order_release);
        assert(beforeValue >= 1 && "resource is released");
        if (beforeValue == 1)
        {
            m_isReleased = true;
            releaseResource();
        }
    }

    bool isReleased() const { return m_isReleased; };
    virtual void releaseResource() = 0;

protected:
    std::atomic<uint16_t> m_refCount = 0;
    bool m_isReleased = false;
};

template <typename ResourceType> class RenderGraphResource : public RenderGraphResourceBase
{
public:
    explicit RenderGraphResource(std::unique_ptr<ResourceType> resource)
        : RenderGraphResourceBase(), m_resource(std::move(resource))
    {}
    ~RenderGraphResource() override { m_resource.reset(); }

    inline ResourceType* getResource() const { return m_resource.get(); }

protected:
    void releaseResource() override { m_resource.reset(); }

protected:
    std::unique_ptr<ResourceType> m_resource;
};

class RenderGraphResourceBaseRef
{
public:
    explicit RenderGraphResourceBaseRef(RenderGraphResourceBase* resource) : m_resource(resource)
    {
        assert(m_resource);
        m_resource->incrementRef();
    }

    virtual ~RenderGraphResourceBaseRef()
    {
        assert(m_resource);
        m_resource->decrementRef();
    }

    // disable copy
    void operator=(const RenderGraphResourceBaseRef& other) = delete;
    void operator=(RenderGraphResourceBaseRef&& other) = delete;
    RenderGraphResourceBaseRef(const RenderGraphResourceBaseRef& other) = delete;
    RenderGraphResourceBaseRef(RenderGraphResourceBaseRef&& other) = delete;

    inline RenderGraphResourceBase* getRGResource() const { return m_resource; }

protected:
    RenderGraphResourceBase* m_resource = nullptr;
};

template <typename ResourceType> class RenderGraphResourceRef : public RenderGraphResourceBaseRef
{
public:
    using Type = ResourceType;

    explicit RenderGraphResourceRef(RenderGraphResource<ResourceType>* resource) : RenderGraphResourceBaseRef(resource)
    {}
    ~RenderGraphResourceRef() override = default;

    inline ResourceType* getInternalResource() const
    {
        return static_cast<RenderGraphResource<ResourceType>>(m_resource)->getResource();
    }
};

} // namespace Render