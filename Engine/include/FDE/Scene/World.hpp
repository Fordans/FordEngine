#pragma once

#include "FDE/Export.hpp"
#include "FDE/Scene/Scene.hpp"
#include "FDE/Scene/Scene2D.hpp"
#include "FDE/Scene/Scene3D.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace FDE
{

/// Top-level container for scenes. Manages scene lifecycle and active scene.
/// Typically one World per application (e.g. EditorWorld, RuntimeWorld).
class FDE_API World
{
  public:
    World() = default;
    ~World() = default;

    World(const World&) = delete;
    World& operator=(const World&) = delete;

    /// Create a new scene. Returns nullptr if name already exists.
    Scene* CreateScene(const std::string& name);

    /// Create a new 2D scene. Returns nullptr if name already exists.
    Scene2D* CreateScene2D(const std::string& name);

    /// Create a new 3D scene. Returns nullptr if name already exists.
    Scene3D* CreateScene3D(const std::string& name);

    /// Get scene by name. Returns nullptr if not found.
    Scene* GetScene(const std::string& name);
    const Scene* GetScene(const std::string& name) const;

    /// Get 2D scene by name. Returns nullptr if not found or not a Scene2D.
    Scene2D* GetScene2D(const std::string& name);
    const Scene2D* GetScene2D(const std::string& name) const;

    /// Get 3D scene by name. Returns nullptr if not found or not a Scene3D.
    Scene3D* GetScene3D(const std::string& name);
    const Scene3D* GetScene3D(const std::string& name) const;

    /// Remove and destroy a scene. Active scene is cleared if it was removed.
    void DestroyScene(const std::string& name);

    /// Get all scene names.
    std::vector<std::string> GetSceneNames() const;

    /// Active scene for update/rendering. May be nullptr.
    Scene* GetActiveScene() { return m_activeScene; }
    const Scene* GetActiveScene() const { return m_activeScene; }

    /// Set the active scene by name. Must exist. Pass empty string to clear.
    void SetActiveScene(const std::string& name);

    /// Set the active scene by pointer. Must be a scene owned by this world.
    void SetActiveScene(Scene* scene);

    /// Calls `OnUpdate` on the active scene only.
    void OnUpdate(float deltaTime);

  private:
    std::unordered_map<std::string, std::unique_ptr<Scene>> m_scenes;
    Scene* m_activeScene = nullptr;
};

} // namespace FDE
