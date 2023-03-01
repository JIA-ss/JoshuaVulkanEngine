#pragma once

#include <glm/fwd.hpp>
#include <glm/glm.hpp>
namespace Util { namespace Math {

class RTMatrix
{
protected:
    glm::vec3 m_rotation = glm::vec3(0);
    glm::vec3 m_position = glm::vec3(0);
    glm::mat4 m_matrix = glm::mat4(1.0f);
    bool m_dirty = true;
protected:
    virtual void updateMatrix();
public:
    RTMatrix(const glm::vec3& r = glm::vec3(0), const glm::vec3& t = glm::vec3(0));

    inline RTMatrix& SetRotation(const glm::vec3& r) { m_rotation = r; m_dirty = true; return *this; }
    inline RTMatrix& SetPosition(const glm::vec3& p) { m_position = p; m_dirty = true; return *this; }

    inline RTMatrix& Rotate(const glm::vec3& r) { m_rotation += r; m_dirty = true; return *this; }
    inline RTMatrix& Translate(const glm::vec3 t) { m_position += t; m_dirty = true; return *this; }

    inline const glm::vec3& GetRotation() { return m_rotation; }
    inline const glm::vec3& GetPosition() { return m_position; }

    const glm::mat4& GetMatrix() { updateMatrix(); return m_matrix; }

    inline bool IsDirty() { return m_dirty; }
};

class ProjectionMatrix
{
public:
    enum Mode
    {
        kPerspective, kOrthogonal
    };
protected:
    glm::mat4 m_projectionMatrix;
    Mode m_mode;
    bool m_dirty = true;
protected:
    virtual void updateMatrix() = 0;
public:
    const glm::mat4& GetProjectionMatrix() { updateMatrix(); return m_projectionMatrix; }
    Mode GetMode() { return m_mode; }
};

class PerspectiveProjectMatrix : public ProjectionMatrix
{
private:
    float m_fovDegree = 45.f;
    float m_aspect;
    float m_near;
    float m_far;
protected:
    void updateMatrix() override;
public:
    PerspectiveProjectMatrix(float fovDegree = 45.f, float aspect = 1.7f, float near = 0.1f, float far = 10.f);
    inline PerspectiveProjectMatrix& SetFovDegree(float fovDegree) { m_fovDegree = fovDegree; m_dirty = true; return *this; }
    inline PerspectiveProjectMatrix& SetAspect(float aspect) { m_aspect = aspect; m_dirty = true; return *this; }
    inline PerspectiveProjectMatrix& SetNear(float near) { m_near = near; m_dirty = true; return *this; }
    inline PerspectiveProjectMatrix& SetFar(float far) { m_far = far; m_dirty = true; return *this; }

    inline float GetFovDegree() { return m_fovDegree; }
    inline float GetAspect() { return m_aspect; }
    inline float GetNear() { return m_near; }
    inline float GetFar() { return m_far; }
};

class OrthogonalProjectMatrix : public ProjectionMatrix
{
private:
    float m_xmin, m_xmax, m_ymin, m_ymax, m_zmin, m_zmax;
protected:
    void updateMatrix() override;
public:
    OrthogonalProjectMatrix(float xmin = -100, float xmax = 100, float ymin = -100, float ymax = 100, float zmin = -100, float zmax = 100);
    inline OrthogonalProjectMatrix& SetXMin(float v) { m_xmin = v; m_dirty = true; return *this; }
    inline OrthogonalProjectMatrix& SetYMin(float v) { m_ymin = v; m_dirty = true; return *this; }
    inline OrthogonalProjectMatrix& SetZMin(float v) { m_zmin = v; m_dirty = true; return *this; }
    inline OrthogonalProjectMatrix& SetXMax(float v) { m_xmin = v; m_dirty = true; return *this; }
    inline OrthogonalProjectMatrix& SetYMax(float v) { m_ymin = v; m_dirty = true; return *this; }
    inline OrthogonalProjectMatrix& SetZMax(float v) { m_zmin = v; m_dirty = true; return *this; }

    inline float GetXMin() { return m_xmin; }
    inline float GetYMin() { return m_ymin; }
    inline float GetZMin() { return m_zmin; }
    inline float GetXMax() { return m_xmin; }
    inline float GetYMax() { return m_ymin; }
    inline float GetZMax() { return m_zmin; }
};

class VPMatrix : public RTMatrix
{
protected:
    glm::vec3 m_upDirection = glm::vec3(0,1,0);
    glm::vec3 m_frontDirection = glm::vec3(0,0,-1);
    OrthogonalProjectMatrix m_orthoMatrix;
    PerspectiveProjectMatrix m_perspMatrix;

    ProjectionMatrix::Mode m_mode;

protected:
    void updateMatrix() override;

public:
    VPMatrix() = default;
    VPMatrix(
        float fovDegree, float aspect, float near = 0.1f, float far = 10.f,
        const glm::vec3& rot = glm::vec3(0),
        const glm::vec3& pos = glm::vec3(0),
        glm::vec3 upDirection = glm::vec3(0,1,0),
        glm::vec3 frontDirection = glm::vec3(0,0,-1)
        );
    VPMatrix(
        float xmin, float xmax, float ymin, float ymax, float zmin, float zmax,
        const glm::vec3& rot = glm::vec3(0),
        const glm::vec3& pos = glm::vec3(0),
        glm::vec3 upDirection = glm::vec3(0,1,0),
        glm::vec3 frontDirection = glm::vec3(0,0,-1)
        );

    PerspectiveProjectMatrix* GetPPerspectiveMatrix();
    OrthogonalProjectMatrix* GetPOrthogonalMatrix();
    const glm::mat4& GetProjMatrix();
    const glm::mat4& GetViewMatrix() { return GetMatrix(); }

    glm::vec3 GetFrontDir();
    float GetNear();
    float GetFar();
};

class SRTMatrix : public RTMatrix
{
protected:
    glm::vec3 m_scale = glm::vec3(1);

protected:
    void updateMatrix() override;

public:
    SRTMatrix(const glm::vec3& s = glm::vec3(1.0f), const glm::vec3& r = glm::vec3(0.0f), const glm::vec3& t = glm::vec3(0.0f), const glm::vec3& center = glm::vec3(0));
    inline SRTMatrix& SetScale(const glm::vec3& s) { m_scale = s; m_dirty = true; return *this; }
    inline const glm::vec3 GetScale() { return m_scale; }
    inline SRTMatrix& Zoom(const glm::vec3& s) { m_scale += s; m_dirty = true; return *this; }
};

}
}