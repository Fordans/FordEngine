#pragma once

#include "FDE/Export.hpp"
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
/// meshAsset: serialization hint for project load, e.g. "builtin:triangle" for default triangle.
struct FDE_API Mesh2DComponent
{
    std::shared_ptr<VertexArray> vertexArray;
    std::string meshAsset;  // "builtin:triangle" or asset path; used when vertexArray is null on load
};

} // namespace FDE
