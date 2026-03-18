#pragma once

#include "FDE/Export.hpp"
#include "FDE/Scene/Scene.hpp"
#include "FDE/Scene/Components.hpp"
#include "FDE/Renderer/Renderer.hpp"
#include "FDE/Renderer/Camera2D.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace FDE
{

/// 2D scene built on entt and the World-Scene-Object architecture.
/// Extends Scene with 2D-specific rendering: entities with Mesh2DComponent
/// and Transform2DComponent are rendered using the given camera.
class FDE_API Scene2D : public Scene
{
  public:
    explicit Scene2D(const std::string& name = "Scene2D");

    /// Render all entities that have both Mesh2DComponent and Transform2DComponent.
    /// \param camera  Camera for view-projection matrix
    /// \param viewportWidth   Viewport width in pixels
    /// \param viewportHeight  Viewport height in pixels
    void Render(const Camera2D& camera, uint32_t viewportWidth, uint32_t viewportHeight);
};

} // namespace FDE
