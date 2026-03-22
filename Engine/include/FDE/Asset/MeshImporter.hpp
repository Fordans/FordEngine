#pragma once

#include "FDE/Export.hpp"
#include <filesystem>
#include <memory>

namespace FDE
{

class VertexArray;

/// Loads triangle meshes via Assimp into engine vertex buffers (position + RGB per vertex).
class FDE_API MeshImporter
{
  public:
    /// Loads the first mesh in the file. Layout matches the default colored mesh shader: Float3 position, Float3 color (white).
    static bool LoadFirstMesh(const std::filesystem::path& path, std::shared_ptr<VertexArray>& outVertexArray);
};

} // namespace FDE
