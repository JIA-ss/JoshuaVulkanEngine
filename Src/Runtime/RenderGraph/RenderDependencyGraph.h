#pragma once

#include <algorithm>
#include <atomic>
#include <set>
#include <stdint.h>
#include <unordered_map>
#include <vector>
#include <assert.h>
namespace Render
{

// Directed Acyclic Graph (DAG) of RenderGraph
class RenderDependencyGraph
{
public:
    class Node
    {
    public:
        virtual ~Node() = default;
        inline uint8_t getRefCount() const { return m_refCount.load(std::memory_order_acquire); }
        inline void incRefCount() { m_refCount.fetch_add(1, std::memory_order_release); }
        inline void decRefCount() { m_refCount.fetch_sub(1, std::memory_order_release); }

    protected:
        std::atomic<uint8_t> m_refCount = 0;
    };

public:
    void addNode(Node* node)
    {
        assert(node && "node must be valid");

        if (auto it = m_sourceToDestMap.find(node); it != m_sourceToDestMap.end())
        {
            assert(false && "Node already exists in graph");
        }
        if (auto it = m_destToSourceMap.find(node); it != m_destToSourceMap.end())
        {
            assert(false && "Node already exists in graph");
        }
        m_sourceToDestMap[node] = std::set<Node*>{};
        m_destToSourceMap[node] = std::set<Node*>{};
        m_nodes.push_back(node);
    }

    void addEdge(Node* from, Node* to)
    {
        assert(from && to && "sourceNode and destNode must be valid");
        auto fromIt = m_sourceToDestMap.find(from);
        auto toIt = m_sourceToDestMap.find(to);
        assert(fromIt != m_sourceToDestMap.end() && toIt != m_sourceToDestMap.end() && "Node does not exist in graph");

        fromIt->second.insert(to);
        m_destToSourceMap[to].insert(from);

        from->incRefCount();
    }

    inline bool isIsolate(Node* node) { return !node || node->getRefCount() == 0; }

    std::set<Node*> cullIsolateNodes()
    {
        std::set<Node*> isolateNodes;
        for (auto& node : m_nodes)
        {
            if (isIsolate(node))
            {
                isolateNodes.insert(node);
            }
        }

        if (isolateNodes.empty())
        {
            return isolateNodes;
        }

        std::vector<Node*> culledNodes;
        culledNodes.reserve(m_nodes.size() - isolateNodes.size());
        for (auto& node : m_nodes)
        {
            if (isolateNodes.find(node) == isolateNodes.end())
            {
                culledNodes.push_back(node);
            }
        }
        m_nodes.swap(culledNodes);

        for (auto& node : isolateNodes)
        {
            m_sourceToDestMap.erase(node);
            m_destToSourceMap.erase(node);
        }
        return isolateNodes;
    }

    const std::set<Node*>& getDestNodes(Node* node)
    {
        if (!isIsolate(node))
        {
            return {};
        }
        if (auto it = m_sourceToDestMap.find(node); it != m_sourceToDestMap.end())
        {
            return it->second;
        }
        return {};
    }

    const std::set<Node*>& getSourceNodes(Node* node)
    {
        if (!isIsolate(node))
        {
            return {};
        }
        if (auto it = m_destToSourceMap.find(node); it != m_destToSourceMap.end())
        {
            return it->second;
        }
        return {};
    }

private:
    std::vector<Node*> m_nodes;
    std::unordered_map<Node*, std::set<Node*>> m_sourceToDestMap;
    std::unordered_map<Node*, std::set<Node*>> m_destToSourceMap;
};

} // namespace Render