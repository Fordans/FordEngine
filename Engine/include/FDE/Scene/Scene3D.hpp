#pragma once

#include "FDE/Export.hpp"
#include "FDE/Scene/Scene.hpp"
#include "FDE/Scene/Components.hpp"
#include "FDE/Renderer/Camera3D.hpp"

namespace FDE
{

class AssetManager;

/// 3D scene: entities with Mesh3DComponent and Transform3DComponent.
class FDE_API Scene3D : public Scene
{
  public:
    explicit Scene3D(const std::string& name = "Scene3D");

    void Render(const Camera3D& camera, uint32_t viewportWidth, uint32_t viewportHeight,
                AssetManager* assets = nullptr);

    /// True if the scene has at least one entity with both Mesh3D and Transform3D (works for any Scene subclass).
    static bool HasMesh3DDrawables(const Scene& scene);

    /// True if any entity has a skybox with a non-empty cross texture path.
    static bool HasSkyboxConfigured(const Scene& scene);

    /// Draw all Mesh3D + Transform3D entities (any `Scene` / subclass).
    static void RenderMesh3DEntities(Scene& scene, const Camera3D& camera, uint32_t viewportWidth,
                                     uint32_t viewportHeight, AssetManager* assets = nullptr);

    /// First matching `SkyboxComponent` with a resolved cubemap (via \p assets).
    static void RenderSkyboxIfAny(Scene& scene, const Camera3D& camera, uint32_t viewportWidth,
                                  uint32_t viewportHeight, AssetManager* assets);
};

} // namespace FDE
