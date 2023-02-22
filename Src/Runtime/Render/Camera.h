#pragma once
#include "Util/Mathutil.h"
#include <glm/geometric.hpp>
#include <glm/glm.hpp>

class Camera
{
public:
    static const glm::vec3 UP;

    Camera(float fovDegree, float aspect, float near = 0.1f, float far = 10.f, const glm::vec3& center = glm::vec3(0));

    inline Util::Math::VPMatrix& GetVPMatrix() { return m_vpMatrix; }
    inline const glm::mat4& GetViewMatrix() { return m_vpMatrix.GetViewMatrix(); }
    inline const glm::mat4& GetProjMatrix() { return m_vpMatrix.GetProjMatrix(); }
    inline glm::vec3 GetDirection() { return glm::normalize(m_vpMatrix.GetCenter() - m_vpMatrix.GetPosition()); }
    inline glm::vec3 GetRight() { return glm::normalize(glm::cross(GetDirection(), UP)); }
    inline glm::vec3 GetUp() { return glm::normalize(glm::cross(GetRight(), GetDirection())); }
    inline const glm::vec3& GetPosition() { return m_vpMatrix.GetPosition(); }
private:
    Util::Math::VPMatrix m_vpMatrix;
};