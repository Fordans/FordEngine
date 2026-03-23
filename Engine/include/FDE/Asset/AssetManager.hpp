#pragma once

#include "FDE/Asset/AssetLoadQueue.hpp"
#include "FDE/Asset/AssetPack.hpp"
#include "FDE/Asset/AssetRegistry.hpp"
#include "FDE/Export.hpp"
#include "FDE/Renderer/Shader.hpp"
#include "FDE/Scene/Components.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>

namespace FDE
{

class VertexArray;

/// GPU texture handle + dimensions (GL texture name).
struct FDE_API Texture2DResource
{
    unsigned int glTextureId = 0;
    int width = 0;
    int height = 0;
    ~Texture2DResource();
};

/// Runtime asset cache: registry + optional .fdepack + refcounted GPU objects.
class FDE_API AssetManager
{
  public:
    AssetManager();
    ~AssetManager();

    void Initialize(const std::string& projectRoot);
    /// Optional: load pack for Player builds (entries override Built/ on miss).
    void SetActivePack(const std::string& absoluteFdepackPath);
    void Shutdown();

    const AssetRegistry& GetRegistry() const { return m_registry; }

    /// Resolve builtin / guid / Assets/ path into vertexArray; increments cache use.
    bool ResolveMesh2D(Mesh2DComponent& mesh);
    bool ResolveMesh3D(Mesh3DComponent& mesh);

    std::shared_ptr<Shader> LoadShader(const AssetId& id);
    std::shared_ptr<Texture2DResource> LoadTexture2D(const AssetId& id);

    /// Process pending async loads (GPU upload on main thread). Call once per frame.
    void ProcessAsyncUploads();

  private:
    std::shared_ptr<VertexArray> LoadMesh2DFromBuilt(const AssetRecord& rec);
    std::shared_ptr<VertexArray> LoadMesh2DFromBlob(const std::vector<uint8_t>& bytes);
    std::string ResolveBuiltAbsolute(const AssetRecord& rec) const;
    const AssetRecord* ResolveRecordFromMeshString(const std::string& meshAsset) const;

    std::string m_projectRoot;
    AssetRegistry m_registry;
    std::unique_ptr<AssetPackReader> m_pack;

    std::unordered_map<std::string, std::shared_ptr<VertexArray>> m_meshCache;
    /// Local-space AABB for imported / cached 3D meshes (key matches \p Mesh3DComponent::meshAsset).
    std::unordered_map<std::string, std::pair<glm::vec3, glm::vec3>> m_mesh3DLocalBoundsCache;
    std::unordered_map<std::string, std::shared_ptr<Shader>> m_shaderCache;
    std::unordered_map<std::string, std::shared_ptr<Texture2DResource>> m_textureCache;

    std::unique_ptr<AssetLoadQueue> m_loadQueue;
};

} // namespace FDE
