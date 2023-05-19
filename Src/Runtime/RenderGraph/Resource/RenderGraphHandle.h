#pragma once

#include <stdint.h>
namespace Render
{

class RenderGraphHandleBase
{
public:
    explicit RenderGraphHandleBase(uint64_t id) : m_id(id) {}
    virtual ~RenderGraphHandleBase() = default;

    static const RenderGraphHandleBase InValid;

    inline bool isValid() const { return m_id != InValid.m_id; }
    inline uint64_t getId() const { return m_id; }
    operator uint64_t() const { return m_id; }

protected:
    // index of resource in RenderGraph
    uint64_t m_id;
};

template <typename ResourceType> class RenderGraphHandle : public RenderGraphHandleBase
{
public:
    using Type = ResourceType;
    explicit RenderGraphHandle(uint64_t id) : RenderGraphHandleBase(id) {}
    explicit RenderGraphHandle(const RenderGraphHandleBase& handle) : RenderGraphHandleBase((uint64_t)handle) {}
};

} // namespace Render
