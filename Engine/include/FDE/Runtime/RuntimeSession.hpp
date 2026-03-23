#pragma once

#include "FDE/Export.hpp"
#include "FDE/Project/ProjectDescriptor.hpp"
#include "FDE/Renderer/Camera3D.hpp"
#include <memory>
#include <string>

namespace FDE
{

class World;
class AssetManager;

/// Owns world, runtime asset cache, and a default fly camera for standalone play.
class FDE_API RuntimeSession
{
  public:
    RuntimeSession();
    ~RuntimeSession();

    RuntimeSession(const RuntimeSession&) = delete;
    RuntimeSession& operator=(const RuntimeSession&) = delete;

    /// Load `.fproject` and project root; initializes `AssetManager` for read-only runtime loads.
    bool LoadFromFProjectFile(const std::string& absoluteFprojectPath, std::string& outError);
    void Shutdown();

    World* GetWorld() { return m_world.get(); }
    const World* GetWorld() const { return m_world.get(); }
    AssetManager* GetAssetManager() { return m_assets.get(); }
    const AssetManager* GetAssetManager() const { return m_assets.get(); }

    Camera3D& GetCamera() { return m_camera; }
    const Camera3D& GetCamera() const { return m_camera; }

    const ProjectDescriptor& GetDescriptor() const { return m_descriptor; }

    /// Matches editor Scene view when `FordEditor.cfg` contains `[Scene] Scene3DNavSensitivity=`; else 1.0.
    float GetScene3DNavSensitivity() const { return m_scene3DNavSensitivity; }

  private:
    ProjectDescriptor m_descriptor;
    std::unique_ptr<World> m_world;
    std::unique_ptr<AssetManager> m_assets;
    Camera3D m_camera;
    float m_scene3DNavSensitivity = 1.0f;
};

} // namespace FDE
