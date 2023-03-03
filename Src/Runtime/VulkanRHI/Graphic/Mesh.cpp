#include "Mesh.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/VulkanDevice.h"
#include <glm/fwd.hpp>
#include <stdint.h>
RHI_NAMESPACE_USING

Mesh::Mesh(VulkanDevice* device, Util::Model::MeshData&& meshData)
    : m_pVulkanDevice(device)
    , m_meshData(std::move(meshData))
{
    auto cmd = m_pVulkanDevice->GetPVulkanCmdPool()->CreateReUsableCmd();
    {
        m_pVulkanVertexBuffer = VulkanVertexBuffer::Create(m_pVulkanDevice, m_meshData.vertices, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_pVulkanVertexBuffer->CopyDataToGPU(cmd, m_pVulkanDevice->GetVkGraphicQueue(), m_meshData.vertices.size() * sizeof(m_meshData.vertices[0]));
        cmd.reset();

        m_pVulkanVertexIndexBuffer = RHI::VulkanVertexIndexBuffer::Create(m_pVulkanDevice, m_meshData.indices, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_pVulkanVertexIndexBuffer->CopyDataToGPU(cmd, m_pVulkanDevice->GetVkGraphicQueue(), m_meshData.indices.size() * sizeof(m_meshData.indices[0]));
        cmd.reset();
    }
    m_pVulkanDevice->GetPVulkanCmdPool()->FreeReUsableCmd(cmd);
}

void Mesh::Bind(vk::CommandBuffer& cmd)
{
    cmd.bindVertexBuffers(0, *m_pVulkanVertexBuffer->GetPVkBuf(), {0});
    cmd.bindIndexBuffer(*m_pVulkanVertexIndexBuffer->GetPVkBuf(), 0, vk::IndexType::eUint32);
}

void Mesh::DrawIndexed(vk::CommandBuffer &cmd)
{
    cmd.drawIndexed(m_meshData.indices.size(), 1, 0,0,0);
}



MeshView::MeshView(Mesh* mesh, VulkanDevice* device)
    : m_pVulkanDevice(device)
    , m_mesh(mesh)
{
    auto cmd = m_pVulkanDevice->GetPVulkanCmdPool()->CreateReUsableCmd();
    {
        m_pVulkanVertexBuffer = VulkanVertexBuffer::Create(m_pVulkanDevice, m_mesh->m_meshData.vertices, vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_pVulkanVertexBuffer->CopyDataToGPU(cmd, m_pVulkanDevice->GetVkGraphicQueue(), m_mesh->m_meshData.vertices.size() * sizeof(m_mesh->m_meshData.vertices[0]));
        cmd.reset();

        m_pVulkanVertexIndexBuffer = RHI::VulkanVertexIndexBuffer::Create(m_pVulkanDevice, m_mesh->m_meshData.indices, vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eDeviceLocal);
        m_pVulkanVertexIndexBuffer->CopyDataToGPU(cmd, m_pVulkanDevice->GetVkGraphicQueue(), m_mesh->m_meshData.indices.size() * sizeof(m_mesh->m_meshData.indices[0]));
        cmd.reset();
    }
    m_pVulkanDevice->GetPVulkanCmdPool()->FreeReUsableCmd(cmd);
}

MeshView::~MeshView()
{

}

void MeshView::Bind(vk::CommandBuffer& cmd)
{
    cmd.bindVertexBuffers(0, *m_pVulkanVertexBuffer->GetPVkBuf(), {0});
    cmd.bindIndexBuffer(*m_pVulkanVertexIndexBuffer->GetPVkBuf(), 0, vk::IndexType::eUint32);
}

void MeshView::DrawIndexed(vk::CommandBuffer& cmd)
{
    cmd.drawIndexed(m_mesh->m_meshData.indices.size(), 1, 0,0,0);
}