#pragma once

#include "FDE/Export.hpp"
#include <glm/glm.hpp>
#include <cstddef>
#include <filesystem>
#include <memory>
#include <string_view>

namespace FDE
{

class VertexArray;

/// Loads triangle meshes via Assimp into engine vertex buffers (position + RGB per vertex).
class FDE_API MeshImporter
{
  public:
    /// Loads every mesh in the scene, merged into one draw call (scene graph transforms applied).
    /// OBJ / multi-object files are fully combined; single-mesh files behave as before.
    /// Layout: Float3 position, Float3 color. Without meaningful per-vertex color variation,
    /// applies directional light using normals.
    /// Optional \p outLocalMin / outLocalMax fill model-space axis-aligned bounds.
    static bool LoadSceneMeshesMerged(const std::filesystem::path& path, std::shared_ptr<VertexArray>& outVertexArray,
                                      glm::vec3* outLocalMin = nullptr, glm::vec3* outLocalMax = nullptr);

    /// Same as \p LoadSceneMeshesMerged but from memory. \p formatHint is a file extension, e.g. ".obj" or "fbx".
    static bool LoadSceneMeshesMergedFromMemory(const void* data, size_t dataSize, std::string_view formatHint,
                                                std::shared_ptr<VertexArray>& outVertexArray,
                                                glm::vec3* outLocalMin = nullptr, glm::vec3* outLocalMax = nullptr);
};

} // namespace FDE
