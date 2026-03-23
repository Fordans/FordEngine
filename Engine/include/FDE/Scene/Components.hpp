#pragma once

#include "FDE/Export.hpp"
#include "FDE/Asset/CubemapResource.hpp"
#include "FDE/Asset/Texture2DResource.hpp"
#include "FDE/Renderer/VertexArray.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <string>

namespace FDE
{

/// Display name for an object (used in hierarchy, serialization).
struct FDE_API TagComponent
{
    std::string name = "Object";
};

/// 2D transform: position, rotation (radians), scale.
/// Used for rendering and physics. Parent-child hierarchy via ParentComponent.
struct FDE_API Transform2DComponent
{
    glm::vec2 position{0.0f, 0.0f};
    float rotation = 0.0f;  // radians
    glm::vec2 scale{1.0f, 1.0f};
};

/// 2D mesh for rendering. Holds a VertexArray (position + color or other attributes).
/// Entities with this component are rendered by the Scene2D renderer.
/// meshAsset: persisted reference — "builtin:triangle", "guid:uuid", "fde://guid/uuid", or "Assets/.../file.fdemesh".
struct FDE_API Mesh2DComponent
{
    std::shared_ptr<VertexArray> vertexArray;
    std::string meshAsset;
};

/// 3D transform: position, rotation (Euler radians XYZ), scale.
struct FDE_API Transform3DComponent
{
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
};

/// Skybox from a horizontal-cross cubemap PNG under `Resources/` (see `FileSystem::ResolveEngineResource`).
/// Use `engine:<file>` e.g. `engine:3d-space-skybox.png`. Empty `crossTextureAsset` disables the skybox.
struct FDE_API SkyboxComponent
{
    std::string crossTextureAsset = "engine:3d-space-skybox.png";
    std::shared_ptr<CubemapResource> cubemap;
};

/// Directional light: `direction` is world-space direction **light rays travel** (e.g. (0,-1,0) = downward).
/// Shading uses the opposite vector (toward the light source). No transform component required.
struct FDE_API DirectionalLightComponent
{
    glm::vec3 direction{0.35f, -0.85f, 0.25f};
    glm::vec3 color{1.0f, 1.0f, 1.0f};
    float intensity = 1.0f;
};

/// 3D mesh: position + normal + RGB albedo + UV (see MeshImporter / builtin cube).
/// meshAsset: "builtin:cube" or "engine:<path>" (Resources/ plus path via FileSystem).
/// \p localBounds* are model-space AABB used for picking (defaults match unit cube).
struct FDE_API Mesh3DComponent
{
    std::shared_ptr<VertexArray> vertexArray;
    std::string meshAsset;
    /// Optional albedo: guid / `Assets/...` path (resolved via AssetManager).
    std::string albedoTextureAsset;
    std::shared_ptr<Texture2DResource> albedoTexture;
    glm::vec3 localBoundsMin{-0.5f, -0.5f, -0.5f};
    glm::vec3 localBoundsMax{0.5f, 0.5f, 0.5f};
};

} // namespace FDE
