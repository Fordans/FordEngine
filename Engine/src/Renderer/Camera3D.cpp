#include "FDE/pch.hpp"
#include "FDE/Renderer/Camera3D.hpp"
#include <cmath>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace FDE
{

Camera3D::Camera3D()
{
    constexpr float yaw = 0.9f;
    constexpr float pitch = 0.35f;
    constexpr float dist = 5.0f;
    const float cp = std::cos(pitch);
    const float sp = std::sin(pitch);
    const float cy = std::cos(yaw);
    const float sy = std::sin(yaw);
    glm::vec3 offset(cp * cy, sp, cp * sy);
    m_position = dist * offset;
    m_yaw = yaw;
    m_pitch = pitch;
}

glm::vec3 Camera3D::GetForward() const
{
    const float cy = std::cos(m_yaw);
    const float sy = std::sin(m_yaw);
    const float cp = std::cos(m_pitch);
    const float sp = std::sin(m_pitch);
    return glm::normalize(glm::vec3(-cy * cp, sp, -sy * cp));
}

glm::mat4 Camera3D::GetViewMatrix() const
{
    const glm::vec3 f = GetForward();
    return glm::lookAt(m_position, m_position + f, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera3D::GetProjectionMatrix(uint32_t viewportWidth, uint32_t viewportHeight) const
{
    if (viewportWidth == 0 || viewportHeight == 0)
        return glm::mat4(1.0f);
    float aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    return glm::perspective(glm::radians(FOV_DEGREES), aspect, NEAR_PLANE, FAR_PLANE);
}

void Camera3D::ClearMouseLookSmoothing()
{
    m_lookSmoothX = 0.f;
    m_lookSmoothY = 0.f;
}

void Camera3D::SetPositionYawPitch(const glm::vec3& position, float yawRadians, float pitchRadians)
{
    m_position = position;
    const double tau = static_cast<double>(glm::two_pi<float>());
    m_yaw = static_cast<float>(std::remainder(static_cast<double>(yawRadians), tau));
    m_pitch = pitchRadians;
    if (m_pitch > MAX_PITCH)
        m_pitch = MAX_PITCH;
    if (m_pitch < MIN_PITCH)
        m_pitch = MIN_PITCH;
    ClearMouseLookSmoothing();
}

void Camera3D::ApplyMouseLook(float deltaX, float deltaY, float sensitivityScale)
{
    constexpr float sens = 0.005f;
    // Blend raw ImGui deltas — cuts high-frequency jitter when rotating quickly; ~0.5 = responsive + stable.
    constexpr float smoothBlend = 0.25f;
    m_lookSmoothX = m_lookSmoothX * (1.f - smoothBlend) + deltaX * smoothBlend;
    m_lookSmoothY = m_lookSmoothY * (1.f - smoothBlend) + deltaY * smoothBlend;

    const float k = sens * sensitivityScale;
    m_yaw += m_lookSmoothX * k;
    m_pitch -= m_lookSmoothY * k;
    if (m_pitch > MAX_PITCH)
        m_pitch = MAX_PITCH;
    if (m_pitch < MIN_PITCH)
        m_pitch = MIN_PITCH;

    // Keep yaw near zero in radians — large angles hurt sin/cos precision and cause visible shimmer.
    const double tau = static_cast<double>(glm::two_pi<float>());
    m_yaw = static_cast<float>(std::remainder(static_cast<double>(m_yaw), tau));
}

void Camera3D::ApplyFlyMovement(float forward, float right, float worldVertical, float deltaTime,
                                float sensitivityScale)
{
    if (deltaTime <= 0.0f)
        return;

    glm::vec2 fr(forward, right);
    if (glm::dot(fr, fr) > 1.0f)
        fr = glm::normalize(fr);
    forward = fr.x;
    right = fr.y;

    constexpr float speed = 6.0f;
    const float step = speed * deltaTime * sensitivityScale;

    glm::vec3 f = GetForward();
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    glm::vec3 r = glm::cross(up, f);
    if (glm::length(r) < 1e-5f)
        r = glm::vec3(1.0f, 0.0f, 0.0f);
    else
        r = glm::normalize(r);

    f = glm::normalize(f);
    m_position += f * (forward * step);
    m_position += r * (right * step);
    m_position += up * (worldVertical * step);
}

void Camera3D::Dolly(float zoomDelta, float sensitivityScale)
{
    if (zoomDelta == 0.0f)
        return;
    constexpr float step = 2.5f;
    m_position += GetForward() * (zoomDelta * step * sensitivityScale);
}

} // namespace FDE
