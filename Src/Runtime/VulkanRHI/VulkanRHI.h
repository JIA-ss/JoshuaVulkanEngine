#pragma once

#include <vector>
#include <memory>
#define RHI_NAMESPACE_BEGIN namespace RHI {
#define RHI_NAMESPACE_END }
#define RHI_NAMESPACE_USING using namespace RHI;


#define BUILDER_SHARED_PTR_SET_FUNC(_BUILDER_TYPE_, _PROP_TYPE_, _PROP_NAME_)   \
protected:    \
    std::shared_ptr<_PROP_TYPE_> m_##_PROP_NAME_ = nullptr; \
public: \
    inline _BUILDER_TYPE_& Set##_PROP_NAME_(std::shared_ptr<_PROP_TYPE_> prop) { m_##_PROP_NAME_ = prop; return *this; }


#define BUILDER_SET_FUNC(_BUILDER_TYPE_, _PROP_TYPE_, _PROP_NAME_, _DEFAULT_VALUE_)   \
protected:    \
    _PROP_TYPE_ m_##_PROP_NAME_ = _DEFAULT_VALUE_; \
public: \
    inline _BUILDER_TYPE_& Set##_PROP_NAME_(const _PROP_TYPE_& prop) { m_##_PROP_NAME_ = prop; return *this; }


RHI_NAMESPACE_BEGIN

#define MAX_FRAMES_IN_FLIGHT 3

class RHIInterface
{
public:
    virtual void Init() = 0;
    virtual void Uninit() = 0;

    virtual ~RHIInterface() = 0;
};

class RHIStack
{
public:
protected:
    std::vector<std::unique_ptr<RHIInterface>> m_pRHIInterfaces;
public:
    RHIStack()
    {
        m_pRHIInterfaces.reserve(10);
    }

    template <typename T, typename ...Args>
    T* Push(Args... args)
    {
        T* derived = new T(args...);
        std::unique_ptr<RHIInterface> p;
        p.reset(derived);
        m_pRHIInterfaces.emplace_back(std::move(p));
        return derived;
    }

    void Pop()
    {
        m_pRHIInterfaces.back()->Uninit();
        m_pRHIInterfaces.pop_back();
    }

    void Init()
    {
        for (auto it = m_pRHIInterfaces.rbegin(); it != m_pRHIInterfaces.rend(); it++)
        {
            it->get()->Init();
        }
    }

    void Uninit()
    {
        for (auto it = m_pRHIInterfaces.rbegin(); it != m_pRHIInterfaces.rend(); it++)
        {
            it->get()->Uninit();
        }
    }
};


RHI_NAMESPACE_END