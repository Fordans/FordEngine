#include "FDE/pch.hpp"
#include "FDE/Renderer/Camera2D.hpp"
#include <algorithm>

namespace FDE
{

glm::mat4 Camera2D::GetViewProjectionMatrix(uint32_t viewportWidth, uint32_t viewportHeight) const
{
    if (viewportWidth == 0 || viewportHeight == 0)
        return glm::mat4(1.0f);

    float aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    float halfWidth = BASE_HALF_SIZE / m_zoom;
    float halfHeight = halfWidth / aspect;

    float left = m_position.x - halfWidth;
    float right = m_position.x + halfWidth;
    float bottom = m_position.y - halfHeight;
    float top = m_position.y + halfHeight;

    glm::mat4 projection = glm::ortho(left, right, bottom, top, -1.0f, 1.0f);
    glm::mat4 view = glm::mat4(1.0f);  // 2D: no rotation, position is already in projection
    return projection * view;
}

void Camera2D::Pan(float deltaX, float deltaY)
{
    m_position.x += deltaX;
    m_position.y += deltaY;
}

void Camera2D::ZoomAt(float zoomDelta, float focusScreenX, float focusScreenY, uint32_t viewportWidth,
                     uint32_t viewportHeight)
{
    if (viewportWidth == 0 || viewportHeight == 0)
        return;

    float aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);
    float halfWidth = BASE_HALF_SIZE / m_zoom;
    float halfHeight = halfWidth / aspect;

    float centerScreenX = static_cast<float>(viewportWidth) * 0.5f;
    float centerScreenY = static_cast<float>(viewportHeight) * 0.5f;

    float worldX = m_position.x + (focusScreenX - centerScreenX) / static_cast<float>(viewportWidth) * (2.0f * halfWidth);
    float worldY = m_position.y - (focusScreenY - centerScreenY) / static_cast<float>(viewportHeight) * (2.0f * halfHeight);

    float newZoom = m_zoom * (1.0f + zoomDelta);
    newZoom = std::clamp(newZoom, MIN_ZOOM, MAX_ZOOM);

    float newHalfWidth = BASE_HALF_SIZE / newZoom;
    float newHalfHeight = newHalfWidth / aspect;

    m_position.x = worldX - (focusScreenX - centerScreenX) / static_cast<float>(viewportWidth) * (2.0f * newHalfWidth);
    m_position.y = worldY + (focusScreenY - centerScreenY) / static_cast<float>(viewportHeight) * (2.0f * newHalfHeight);
    m_zoom = newZoom;
}

void Camera2D::SetZoom(float zoom)
{
    m_zoom = std::clamp(zoom, MIN_ZOOM, MAX_ZOOM);
}

} // namespace FDE
