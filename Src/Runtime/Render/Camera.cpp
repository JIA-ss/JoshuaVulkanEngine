#include "Camera.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Util/Mathutil.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <vcruntime.h>

const glm::vec3 Camera::UP = glm::vec3(0,1,0);

Camera::Camera(float fovDegree, float aspect, float near, float far)
    : m_vpMatrix(fovDegree, aspect, near, far, glm::vec3(0), glm::vec3(0))
{

}


void Camera::InitUniformBuffer(RHI::VulkanDevice* device)
{
    for (int frameId = 0; frameId < m_uniformBuffers.size(); frameId++)
    {
        m_uniformBuffers[frameId].reset(
        new RHI::VulkanBuffer(
                device, sizeof(RHI::CameraUniformBufferObject),
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                vk::SharingMode::eExclusive
            )
        );
    }
}

void Camera::UpdateUniformBuffer(int frameId)
{
    RHI::CameraUniformBufferObject ubo;
    ubo.camPos = glm::vec4(m_vpMatrix.GetPosition(), 1.0f);
    ubo.proj = m_vpMatrix.GetProjMatrix();
    ubo.view = m_vpMatrix.GetViewMatrix();

    constexpr const size_t ubosize = sizeof(ubo);

    if (m_uniformBufferObjects[frameId].camPos != ubo.camPos
        || m_uniformBufferObjects[frameId].proj != ubo.proj
        || m_uniformBufferObjects[frameId].view != ubo.view
    )
    {
        m_uniformBufferObjects[frameId] = ubo;
        m_uniformBuffers[frameId]->FillingMappingBuffer(&ubo, 0, ubosize);
    }
}

void Camera::SetUniformBufferObject(int frameId, RHI::CameraUniformBufferObject* ubo)
{
    m_uniformBuffers[frameId]->FillingMappingBuffer(ubo, 0, sizeof(RHI::CameraUniformBufferObject));
    m_uniformBufferObjects[frameId] = *ubo;
}


std::array<RHI::Model::UBOLayoutInfo, MAX_FRAMES_IN_FLIGHT> Camera::GetUboInfo()
{
    std::array<RHI::Model::UBOLayoutInfo, MAX_FRAMES_IN_FLIGHT> uboInfo;
    for (int frameId = 0; frameId < uboInfo.size(); frameId++)
    {
        uboInfo[frameId] = RHI::Model::UBOLayoutInfo
        {
            m_uniformBuffers[frameId].get(),
            RHI::VulkanDescriptorSetLayout::DESCRIPTOR_CAMVPUBO_BINDING_ID,
            sizeof(RHI::CameraUniformBufferObject)
        };
    };
    return uboInfo;
}