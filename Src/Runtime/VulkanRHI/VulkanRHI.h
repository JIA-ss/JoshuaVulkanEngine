#pragma once

#include <vector>
#include <memory>
#define RHI_NAMESPACE_BEGIN                                                                                            \
    namespace RHI                                                                                                      \
    {
#define RHI_NAMESPACE_END }
#define RHI_NAMESPACE_USING using namespace RHI;

#define BUILDER_SHARED_PTR_SET_FUNC(_BUILDER_TYPE_, _PROP_TYPE_, _PROP_NAME_)                                          \
protected:                                                                                                             \
    std::shared_ptr<_PROP_TYPE_> m_##_PROP_NAME_ = nullptr;                                                            \
                                                                                                                       \
public:                                                                                                                \
    inline _BUILDER_TYPE_& Set##_PROP_NAME_(std::shared_ptr<_PROP_TYPE_> prop)                                         \
    {                                                                                                                  \
        m_##_PROP_NAME_ = prop;                                                                                        \
        return *this;                                                                                                  \
    }

#define BUILDER_SET_FUNC(_BUILDER_TYPE_, _PROP_TYPE_, _PROP_NAME_, _DEFAULT_VALUE_)                                    \
protected:                                                                                                             \
    _PROP_TYPE_ m_##_PROP_NAME_ = _DEFAULT_VALUE_;                                                                     \
                                                                                                                       \
public:                                                                                                                \
    inline _BUILDER_TYPE_& Set##_PROP_NAME_(const _PROP_TYPE_& prop)                                                   \
    {                                                                                                                  \
        m_##_PROP_NAME_ = prop;                                                                                        \
        return *this;                                                                                                  \
    }

RHI_NAMESPACE_BEGIN

#define MAX_FRAMES_IN_FLIGHT 3

RHI_NAMESPACE_END