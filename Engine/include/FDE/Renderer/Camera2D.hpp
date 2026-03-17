#pragma once

#include "FDE/Export.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdint>

namespace FDE
{

/// Orthographic 2D camera for scene viewport navigation.
/// - Right mouse drag: pan (change camera position)
/// - Mouse wheel: zoom toward cursor
class FDE_API Camera2D
{
  public:
    Camera2D() = default;

    /// Get the combined view-projection matrix for rendering.
    /// \param viewportWidth  Viewport width in pixels
    /// \param viewportHeight Viewport height in pixels
    glm::mat4 GetViewProjectionMatrix(uint32_t viewportWidth, uint32_t viewportHeight) const;

    /// Pan the camera by delta in world units (positive = right/up).
    void Pan(float deltaX, float deltaY);

    /// Zoom toward a screen-space point. Keeps the point under the cursor fixed.
    /// \param zoomDelta     Positive = zoom in, negative = zoom out
    /// \param focusScreenX  X position of zoom focus in viewport pixels (0 = left)
    /// \param focusScreenY  Y position of zoom focus in viewport pixels (0 = top)
    /// \param viewportWidth  Viewport width in pixels
    /// \param viewportHeight Viewport height in pixels
    void ZoomAt(float zoomDelta, float focusScreenX, float focusScreenY, uint32_t viewportWidth,
                uint32_t viewportHeight);

    glm::vec2 GetPosition() const { return m_position; }
    void SetPosition(const glm::vec2& pos) { m_position = pos; }

    float GetZoom() const { return m_zoom; }
    void SetZoom(float zoom);

  private:
    static constexpr float MIN_ZOOM = 0.1f;
    static constexpr float MAX_ZOOM = 100.0f;
    /// At zoom 1, half-width in world units (height is aspect-corrected)
    static constexpr float BASE_HALF_SIZE = 2.0f;

    glm::vec2 m_position{0.0f, 0.0f};
    float m_zoom = 1.0f;
};

} // namespace FDE
