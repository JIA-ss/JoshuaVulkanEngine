#include "Mathutil.h"

#define GLM_DEPTH_ZERO_TO_ONE
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/fwd.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/matrix.hpp>
#include <iostream>

namespace Util {

Math::RTMatrix::RTMatrix(const glm::vec3& r, const glm::vec3& t)
    : m_rotation(r)
    , m_position(t)
    , m_dirty(true)
{

}
void Math::RTMatrix::updateMatrix()
{
    if (!m_dirty)
    {
        return;
    }


    glm::mat4 xrot = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.x), glm::vec3(1,0,0));
    glm::mat4 yrot = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.y), glm::vec3(0,1,0));
    glm::mat4 zrot = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.z), glm::vec3(0,0,1));
    glm::mat4 trot = glm::translate(glm::mat4(1.0f), m_position);

    m_matrix = trot * zrot * yrot * xrot;
    m_dirty = false;
}

Math::PerspectiveProjectMatrix::PerspectiveProjectMatrix(float fovDegree, float aspect, float near, float far)
    : m_aspect(aspect)
    , m_fovDegree(fovDegree)
    , m_near(near)
    , m_far(far)
{
    m_mode = kPerspective;
}

void Math::PerspectiveProjectMatrix::updateMatrix()
{
    if (!m_dirty)
    {
        return;
    }
    m_projectionMatrix = glm::perspective(glm::radians(m_fovDegree), m_aspect, m_near, m_far);
    m_projectionMatrix[1][1] *= -1;

    // std::cout << "proj: " << glm::to_string(m_projectionMatrix) << std::endl;
    m_dirty = false;
}

Math::OrthogonalProjectMatrix::OrthogonalProjectMatrix(float xmin, float xmax, float ymin, float ymax, float zmin, float zmax)
    : m_xmin(xmin)
    , m_xmax(xmax)
    , m_ymin(ymin)
    , m_ymax(ymax)
    , m_zmin(zmin)
    , m_zmax(zmax)
{
    
}

void Math::OrthogonalProjectMatrix::updateMatrix()
{
    if (!m_dirty)
    {
        return;
    }

    m_projectionMatrix = glm::ortho(m_xmin, m_xmax, m_ymin, m_ymax, m_zmin, m_zmax);
    std::cout << "ortho param: " << m_xmin << " " << m_xmax << " " << m_ymin << " " << m_ymax << " " << m_zmin << " " << m_zmax << std::endl;
    std::cout << "ortho matrix: " << glm::to_string(m_projectionMatrix) << std::endl;
    m_projectionMatrix[1][1] *= -1;
    m_dirty = false;
}


Math::VPMatrix::VPMatrix(
    float fovDegree, float aspect, float near, float far,
    const glm::vec3& rot,
    const glm::vec3& pos,
    glm::vec3 upDirection,
    glm::vec3 frontDirection
)
    : m_mode(ProjectionMatrix::Mode::kPerspective)
    , m_perspMatrix(fovDegree, aspect, near, far)
    , m_upDirection(upDirection)
    , m_frontDirection(frontDirection)
    , RTMatrix(rot, pos)
{
    glm::vec3 v3front = GetFrontDir();
    glm::vec3 right = glm::cross(v3front, m_upDirection);
    m_upDirection = glm::cross(right, v3front);
}

Math::VPMatrix::VPMatrix(
        float xmin, float xmax, float ymin, float ymax, float zmin, float zmax,
        const glm::vec3& rot ,
        const glm::vec3& pos,
        glm::vec3 upDirection ,
        glm::vec3 frontDirection
)
    : m_mode(ProjectionMatrix::Mode::kOrthogonal)
    , m_orthoMatrix(xmin, xmax, ymin, ymax, zmin, zmax)
    , m_upDirection(upDirection)
    , m_frontDirection(frontDirection)
    , RTMatrix(rot, pos)
{
    glm::vec3 v3front = GetFrontDir();
    glm::vec3 right = glm::cross(v3front, m_upDirection);
    m_upDirection = glm::cross(right, v3front);
}

glm::vec3 Math::VPMatrix::GetFrontDir()
{
    glm::mat4 xrot = glm::rotate(glm::mat4(1.0), glm::radians(m_rotation.x), glm::vec3(1,0,0));
    glm::mat4 yrot = glm::rotate(glm::mat4(1.0), glm::radians(m_rotation.y), glm::vec3(0,1,0));
    glm::mat4 zrot = glm::rotate(glm::mat4(1.0), glm::radians(m_rotation.z), glm::vec3(0,0,1));
    glm::vec4 front = zrot * yrot * xrot * glm::vec4(m_frontDirection, 1);
    glm::vec3 v3front(front.x, front.y, front.z);
    return glm::normalize(v3front);
}

float Math::VPMatrix::GetNear()
{
    if (m_mode == ProjectionMatrix::Mode::kPerspective)
    {
        return m_perspMatrix.GetNear();
    }
    return 0.0f;
}
float Math::VPMatrix::GetFar()
{
    if (m_mode == ProjectionMatrix::Mode::kPerspective)
    {
        return m_perspMatrix.GetFar();
    }
    return 0.0f;
}

Math::PerspectiveProjectMatrix* Math::VPMatrix::GetPPerspectiveMatrix()
{
    if (m_mode == ProjectionMatrix::kPerspective)
    {
        return &m_perspMatrix;
    }
    return nullptr;
}
Math::OrthogonalProjectMatrix* Math::VPMatrix::GetPOrthogonalMatrix()
{
    if (m_mode == ProjectionMatrix::kOrthogonal)
    {
        return &m_orthoMatrix;
    }
    return nullptr;
}

const glm::mat4& Math::VPMatrix::GetProjMatrix()
{
    switch (m_mode) {
    case ProjectionMatrix::kPerspective:
    {
        return m_perspMatrix.GetProjectionMatrix();
    }
    case ProjectionMatrix::kOrthogonal:
    {
        return m_orthoMatrix.GetProjectionMatrix();
    }
    }
}

void Math::VPMatrix::updateMatrix()
{
    if (!m_dirty)
    {
        return;
    }

    // glm::mat4 v = glm::translate(glm::mat4(1.0f), m_position);
    // v = glm::rotate(v, glm::radians(m_rotation.x), glm::vec3(1,0,0));
    // v = glm::rotate(v, glm::radians(m_rotation.y), glm::vec3(0,1,0));
    // v = glm::rotate(v, glm::radians(m_rotation.z), glm::vec3(0,0,1));
    // m_matrix = glm::inverse(v);

    glm::vec3 v3front = GetFrontDir();
    glm::vec3 right = glm::cross(v3front, m_upDirection);
    m_upDirection = glm::cross(right, v3front);
    m_matrix = glm::lookAt(m_position, m_position + v3front * 10.f, m_upDirection);

    // std::cout << "view: " << glm::to_string(m_matrix) << std::endl;
    m_dirty = false;
}


Math::SRTMatrix::SRTMatrix(const glm::vec3& s, const glm::vec3& r, const glm::vec3& t, const glm::vec3& center)
    : m_scale(s)
    , RTMatrix(r,t)
{

}

void Math::SRTMatrix::updateMatrix()
{
    if (!m_dirty)
    {
        return;
    }


    glm::mat4 scalemat = glm::scale(glm::mat4(1.0f), m_scale);
    glm::mat4 xrotmat = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.x), glm::vec3(1,0,0));
    glm::mat4 yrotmat = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.y), glm::vec3(0,1,0));
    glm::mat4 zrotmat = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotation.z), glm::vec3(0,0,1));
    glm::mat4 tmat = glm::translate(glm::mat4(1.0f), m_position);
    m_matrix = tmat * zrotmat * yrotmat * xrotmat * scalemat;

    // std::cout << "scale: " << glm::to_string(m_scale) << std::endl;
    // std::cout << "rot: " << glm::to_string(m_rotation) << std::endl;
    // std::cout << "translate: " << glm::to_string(m_position) << std::endl;
    // std::cout << "matrix" << std::endl << glm::to_string(m_matrix) << std::endl;

    m_dirty = false;
}

}