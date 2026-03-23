#pragma once

#include "FDE/Export.hpp"
#include "FDE/Scene/Object.hpp"
#include <entt.hpp>
#include <memory>
#include <string>

namespace FDE
{

/// A scene owns an ECS registry and its entities (objects).
/// One scene is typically active at a time for update/rendering.
class FDE_API Scene
{
  public:
    explicit Scene(const std::string& name = "Scene");
    virtual ~Scene();

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    /// Create a new object (entity) in this scene.
    Object CreateObject();

    /// Destroy an object. Invalidates the Object handle.
    void DestroyObject(Object object);

    /// Check if an object is valid in this scene.
    bool IsValid(Object object) const;

    /// Direct access to the ECS registry for advanced usage.
    entt::registry& GetRegistry() { return m_registry; }
    const entt::registry& GetRegistry() const { return m_registry; }

    /// Scene display name.
    const std::string& GetName() const { return m_name; }
    void SetName(const std::string& name) { m_name = name; }

    /// Per-frame simulation (gameplay). Editor may skip when paused.
    virtual void OnUpdate(float /*deltaTime*/) {}

    // --- Component helpers (convenience over registry.emplace/get) ---

    template<typename T, typename... Args>
    T& AddComponent(Object object, Args&&... args)
    {
        return m_registry.emplace<T>(object.GetEntity(), std::forward<Args>(args)...);
    }

    template<typename T>
    T* GetComponent(Object object)
    {
        return m_registry.try_get<T>(object.GetEntity());
    }

    template<typename T>
    const T* GetComponent(Object object) const
    {
        return m_registry.try_get<const T>(object.GetEntity());
    }

    template<typename T>
    bool HasComponent(Object object) const
    {
        return m_registry.all_of<T>(object.GetEntity());
    }

    template<typename T>
    void RemoveComponent(Object object)
    {
        m_registry.remove<T>(object.GetEntity());
    }

  private:
    entt::registry m_registry;
    std::string m_name;
};

} // namespace FDE
