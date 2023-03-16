#pragma once
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Runtime/VulkanRHI/Layout/UniformBufferObject.h"
#include "Runtime/VulkanRHI/Resources/VulkanBuffer.h"
#include "Runtime/VulkanRHI/VulkanRHI.h"
#include "Runtime/VulkanRHI/Graphic/Model.h"
#include "Util/Mathutil.h"
#include <glm/geometric.hpp>
#include <glm/glm.hpp>

class Camera
{
public:
    static const glm::vec3 UP;

    Camera(float fovDegree, float aspect, float near = 0.1f, float far = 10.f);

    inline Util::Math::VPMatrix& GetVPMatrix() { return m_vpMatrix; }
    inline const glm::mat4& GetViewMatrix() { return m_vpMatrix.GetViewMatrix(); }
    inline const glm::mat4& GetProjMatrix() { return m_vpMatrix.GetProjMatrix(); }
    inline glm::vec3 GetDirection() { return glm::normalize(m_vpMatrix.GetFrontDir()); }
    inline glm::vec3 GetRight() { return glm::normalize(glm::cross(GetDirection(), UP)); }
    inline glm::vec3 GetUp() { return glm::normalize(glm::cross(GetRight(), GetDirection())); }
    inline const glm::vec3& GetPosition() { return m_vpMatrix.GetPosition(); }

    void InitUniformBuffer(RHI::VulkanDevice* device);
    void UpdateUniformBuffer();
    void SetUniformBufferObject(RHI::CameraUniformBufferObject* ubo);
    glm::mat4 GetModelMatrix();
    RHI::Model::UBOLayoutInfo GetUboInfo();
private:
    Util::Math::VPMatrix m_vpMatrix;
    std::unique_ptr<RHI::VulkanBuffer> m_uniformBuffer;
    RHI::CameraUniformBufferObject m_uniformBufferObject;
};