#include "Camera.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Layout/VulkanDescriptorSetLayout.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Util/Mathutil.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <vcruntime.h>

const glm::vec3 Camera::UP = glm::vec3(0,1,0);

Camera::Camera(float fovDegree, float aspect, float near, float far)
    : m_vpMatrix(fovDegree, aspect, near, far, glm::vec3(0), glm::vec3(0))
{

}


void Camera::InitUniformBuffer(RHI::VulkanDevice* device)
{
    {
        m_uniformBuffer.reset(
        new RHI::VulkanBuffer(
                device, sizeof(RHI::CameraUniformBufferObject),
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
                vk::SharingMode::eExclusive
            )
        );
    }
}

void Camera::UpdateUniformBuffer()
{
    ZoneScopedN("Camera::UpdateUniformbuffer");
    RHI::CameraUniformBufferObject ubo;
    ubo.camPos = glm::vec4(m_vpMatrix.GetPosition(), 1.0f);
    ubo.proj = m_vpMatrix.GetProjMatrix();
    ubo.view = m_vpMatrix.GetViewMatrix();
    ubo.model = GetModelMatrix();

    constexpr const size_t ubosize = sizeof(ubo);

    if (m_uniformBufferObject.camPos != ubo.camPos
        || m_uniformBufferObject.proj != ubo.proj
        || m_uniformBufferObject.view != ubo.view
        || m_uniformBufferObject.model != ubo.model
    )
    {
        m_uniformBufferObject = ubo;
        m_uniformBuffer->FillingMappingBuffer(&ubo, 0, ubosize);
    }
}

void Camera::SetUniformBufferObject(RHI::CameraUniformBufferObject* ubo)
{
    m_uniformBuffer->FillingMappingBuffer(ubo, 0, sizeof(RHI::CameraUniformBufferObject));
    m_uniformBufferObject = *ubo;
}

glm::mat4 Camera::GetModelMatrix()
{
    auto rotation = m_vpMatrix.GetRotation();
    auto translate = GetPosition();

    glm::mat4 xrot = glm::rotate(glm::mat4(1), glm::radians(rotation.x), glm::vec3(1,0,0));
    glm::mat4 yrot = glm::rotate(glm::mat4(1), glm::radians(rotation.y), glm::vec3(0,1,0));
    glm::mat4 zrot = glm::rotate(glm::mat4(1), glm::radians(rotation.z), glm::vec3(0,0,1));
    glm::mat4 trot = glm::translate(glm::mat4(1), translate);
    return trot * zrot * yrot * xrot;
}

RHI::Model::UBOLayoutInfo Camera::GetUboInfo()
{
    RHI::Model::UBOLayoutInfo uboInfo = RHI::Model::UBOLayoutInfo
    {
        m_uniformBuffer.get(),
        RHI::VulkanDescriptorSetLayout::DESCRIPTOR_CAMVPUBO_BINDING_ID,
        sizeof(RHI::CameraUniformBufferObject)
    };

    return uboInfo;
}