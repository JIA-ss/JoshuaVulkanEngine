#pragma once
#include "Runtime/VulkanRHI/Graphic/Vertex.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Util/Modelutil.h"
#include <boost/filesystem/path.hpp>
#include <vulkan/vulkan.hpp>
RHI_NAMESPACE_BEGIN

class MeshView;
class Mesh final
{
    friend class Model;
    friend class MeshView;
private:
    VulkanDevice* m_pVulkanDevice;
    Util::Model::MeshData m_meshData;

    std::unique_ptr<VulkanVertexBuffer> m_pVulkanVertexBuffer;
    std::unique_ptr<VulkanVertexIndexBuffer> m_pVulkanVertexIndexBuffer;
public:
    explicit Mesh(VulkanDevice* device, Util::Model::MeshData&& meshData);

    void Bind(vk::CommandBuffer& cmd);
    void DrawIndexed(vk::CommandBuffer& cmd);
private:

};

class MeshView
{
private:
    Mesh* m_mesh;
    VulkanDevice* m_pVulkanDevice;

    std::unique_ptr<VulkanVertexBuffer> m_pVulkanVertexBuffer;
    std::unique_ptr<VulkanVertexIndexBuffer> m_pVulkanVertexIndexBuffer;
public:
    explicit MeshView(Mesh* mesh, VulkanDevice* device);
    ~MeshView();

    void Bind(vk::CommandBuffer& cmd);
    void DrawIndexed(vk::CommandBuffer& cmd);
};

RHI_NAMESPACE_END