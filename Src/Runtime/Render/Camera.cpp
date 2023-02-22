#include "Camera.h"
#include "Util/Mathutil.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

const glm::vec3 Camera::UP = glm::vec3(0,1,0);

Camera::Camera(float fovDegree, float aspect, float near, float far, const glm::vec3& center)
    : m_vpMatrix(fovDegree, aspect, near, far, glm::vec3(0), glm::vec3(0), center, UP)
{

}