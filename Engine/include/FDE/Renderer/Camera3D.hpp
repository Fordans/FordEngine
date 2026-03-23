#pragma once

#include "FDE/Export.hpp"
#include <glm/glm.hpp>
#include <cstdint>

namespace FDE
{

/// First-person style camera for the 3D scene view: position + yaw/pitch.
class FDE_API Camera3D
{
  public:
    Camera3D();

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix(uint32_t viewportWidth, uint32_t viewportHeight) const;

    /// Mouse look while holding RMB (pixel deltas from ImGui). \p sensitivityScale multiplies base speed (default 1).
    void ApplyMouseLook(float deltaX, float deltaY, float sensitivityScale = 1.0f);

    /// Reset smoothed mouse-look state (call when RMB look is not active).
    void ClearMouseLookSmoothing();

    /// Move in view space (forward/right) and world Y. Each axis in [-1, 1].
    void ApplyFlyMovement(float forward, float right, float worldVertical, float deltaTime,
                          float sensitivityScale = 1.0f);

    /// Mouse wheel: move along view forward.
    void Dolly(float zoomDelta, float sensitivityScale = 1.0f);

    glm::vec3 GetPosition() const { return m_position; }
    void SetPosition(const glm::vec3& p) { m_position = p; }

    float GetYaw() const { return m_yaw; }
    float GetPitch() const { return m_pitch; }

    /// Sets pose from persisted editor/runtime view; clamps pitch and clears mouse-look smoothing.
    void SetPositionYawPitch(const glm::vec3& position, float yawRadians, float pitchRadians);

    /// Normalized world-space view direction (into the scene).
    glm::vec3 GetForward() const;

  private:

    static constexpr float MIN_PITCH = -1.553343f;
    static constexpr float MAX_PITCH = 1.553343f;
    static constexpr float FOV_DEGREES = 45.0f;
    static constexpr float NEAR_PLANE = 0.05f;
    static constexpr float FAR_PLANE = 256.0f;

    glm::vec3 m_position{0.0f, 0.0f, 0.0f};
    float m_yaw = 0.9f;
    float m_pitch = 0.35f;
    /// Low-pass filtered mouse deltas (reduces frame-to-frame noise when spinning fast).
    float m_lookSmoothX = 0.f;
    float m_lookSmoothY = 0.f;
};

} // namespace FDE
