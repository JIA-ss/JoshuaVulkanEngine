#pragma once

#include <vector>
#include <memory>
#define RHI_NAMESPACE_BEGIN namespace RHI {
#define RHI_NAMESPACE_END }
#define RHI_NAMESPACE_USING using namespace RHI;

RHI_NAMESPACE_BEGIN

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